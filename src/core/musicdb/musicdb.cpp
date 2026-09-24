#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <fmt/base.h>
#include "collection.hpp"
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/search_utils.hpp"
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/musicdb/types.hpp"
#include "core/track_file.hpp"
#include "musicdb.hpp"
#include "playlist.hpp"
#include "track.hpp"

using db::Collection;
using db::Playlist;
using db::Track;

static std::vector<db::Collection> collections;
static std::vector<db::Playlist> playlists;
static std::vector<db::Track> tracks;
static std::unordered_map<std::filesystem::path, size_t> path_to_track_id;
static std::unordered_map<std::string, std::unordered_set<db::track_id_t>> title_to_track_ids;
static std::unordered_map<std::filesystem::path, std::unordered_set<db::track_id_t>> file_name_to_track_ids;
static std::vector<std::pair<db::collection_id_t, db::track_id_t>> orphaned_tracks;

static constexpr size_t DB_MAGIC = 667667667;
static constexpr size_t DB_VERSION = 3;

void db::create_empty_db() {
  collections.clear();
  collections.clear();
  playlists.clear();
  tracks.clear();
  path_to_track_id.clear();
  title_to_track_ids.clear();
  file_name_to_track_ids.clear();
  orphaned_tracks.clear();
  db::add_collection("Playlists");
  db::add_playlist_to_collection(0, db::Playlist{"Loved tracks", {""}, db::PlaylistType::User});
}

void db::serialize(std::ofstream& os) {
  write_bin(os, DB_MAGIC);
  write_bin(os, DB_VERSION);
  // Initially mark all tracks and playlists as tombstone, and keep track of playlists
  // removed by user
  for (size_t track_id = 0; track_id < tracks.size(); track_id += 1) {
    tracks[track_id].set_flag(TOMBSTONE);
  }
  std::unordered_set<size_t> playlist_ids_removed_by_user_or_empty;
  for (size_t playlist_id = 1; playlist_id < playlists.size(); playlist_id += 1) {
    if (playlists[playlist_id].is_tombstone() || playlists[playlist_id].get_tracks_count() == 0) {
      playlist_ids_removed_by_user_or_empty.insert(playlist_id);
    }
    playlists[playlist_id].set_tombstone(true);
  }

  // Untombstone tracks and playlists present in some non-tombstoned playlist/album
  for (size_t collection_id = 0; collection_id < collections.size(); collection_id += 1) {
    if (collections[collection_id].is_tombstone()) { continue; }
    for (size_t playlist_id : collections[collection_id].playlist_ids()) {

      // Check if playlist was removed by user, and if so skip it (keep it tombstoned)
      if (playlist_ids_removed_by_user_or_empty.contains(playlist_id)) { continue; }

      auto& playlist = playlists[playlist_id];
      playlist.set_tombstone(false);
      for (size_t track_id : playlist.get_track_ids()) {
        tracks[track_id].unset_flag(TOMBSTONE);
      }
    }
  }

  std::vector<size_t> old_track_id_to_new_track_id(tracks.size(), INVALID_ID);
  size_t nontombstoned_tracks_count = 0;
  for (size_t old_track_id = 0; old_track_id < tracks.size(); old_track_id += 1) {
    if (!tracks[old_track_id].get_flag(TOMBSTONE)) {
      old_track_id_to_new_track_id[old_track_id] = nontombstoned_tracks_count;
      nontombstoned_tracks_count += 1;
    }
  }

  std::vector<size_t> old_playlist_id_to_new_playlist_id(playlists.size(), INVALID_ID);
  size_t nontombstoned_playlists_count = 0;
  for (size_t old_playlist_id = 0; old_playlist_id < playlists.size(); old_playlist_id += 1) {
    if (!playlists[old_playlist_id].is_tombstone()) {
      old_playlist_id_to_new_playlist_id[old_playlist_id] = nontombstoned_playlists_count;
      nontombstoned_playlists_count += 1;
    }
  }

  size_t nontombstoned_collections_count = 0;
  for (size_t old_collection_id = 0; old_collection_id < collections.size(); old_collection_id += 1) {
    if (!collections[old_collection_id].is_tombstone()) { nontombstoned_collections_count += 1; }
  }

  write_bin(os, nontombstoned_collections_count);
  for (auto& c : collections) {
    if (c.is_tombstone()) { continue; }
    c.serialize(os, old_playlist_id_to_new_playlist_id);
  }

  write_bin(os, nontombstoned_playlists_count);
  for (auto& p : playlists) {
    if (p.is_tombstone()) { continue; }
    p.serialize(os, old_track_id_to_new_track_id);
  }

  write_bin(os, nontombstoned_tracks_count);
  for (auto& t : tracks) {
    if (t.get_flag(TOMBSTONE)) { continue; }
    t.serialize(os, old_playlist_id_to_new_playlist_id);
  }
}

void db::deserialize(std::ifstream& is) {
  size_t db_magic{};
  read_bin(is, db_magic);
  if (db_magic != DB_MAGIC) {
    out::critical("Database corrupted: invalid DB_MAGIC");
    exit(1);
  }
  size_t db_version{};
  read_bin(is, db_version);
  if (db_version != DB_VERSION) {
    out::critical("Database version number mismatch! Migration currently not supported");
    exit(1);
  }

  collections.clear();
  playlists.clear();
  tracks.clear();
  path_to_track_id.clear();
  title_to_track_ids.clear();
  file_name_to_track_ids.clear();
  orphaned_tracks.clear();

  size_t collections_size = 0;
  read_bin(is, collections_size);
  if (collections_size > 65536) {
    out::critical("Database corrupted: invalid collections_size");
    exit(1);
  }
  for (size_t i = 0; i < collections_size; i += 1) {
    collections.emplace_back(Collection{is});
  }

  size_t playlists_size = 0;
  read_bin(is, playlists_size);
  if (playlists_size > 65536) {
    out::critical("Database corrupted: invalid playlists_size");
    exit(1);
  }
  for (size_t i = 0; i < playlists_size; i += 1) {
    playlists.emplace_back(Playlist{is});
  }

  size_t tracks_size = 0;
  read_bin(is, tracks_size);
  if (tracks_size > 1048576) {
    out::critical("Database corrupted: invalid tracks_size");
    exit(1);
  }
  for (size_t i = 0; i < tracks_size; i += 1) {
    tracks.emplace_back(Track{is});
    Track& t = tracks.back();
    track_id_t track_id = tracks.size() - 1;
    title_to_track_ids[t.metadata.title].insert(track_id);
    path_to_track_id[t.file_path] = track_id;
    file_name_to_track_ids[t.file_name_without_extension()].insert(track_id);
  }

  for (auto& p : playlists) {
    if (p.type == PlaylistType::Album) { p.sort_by_track_number(); }
  }
}

void db::print_collections() {
  out::println("Collection count: {}", collections.size());
  out::println("Playlist count: {}", playlists.size());
  out::println("Track count: {}", tracks.size());
  out::println("Database tree:");
  for (auto& c : collections) {
    out::println("Collection {}", c.name());
    for (auto p_id : c.playlist_ids()) {
      auto& p = playlists[p_id];
      if (!p.tombstone) {
        out::println("\tPlaylist {} - {}", p.name, p.author_pretty());
      } else {
        out::println("\tPlaylist {} - {} (tombstone)", p.name, p.author_pretty());
      }
      for (size_t t_id : p.get_track_ids()) {
        auto& t = tracks[t_id];
        if (!t.get_flag(TOMBSTONE)) {
          out::println(("\t\t{}. {}, {}"), t.metadata.track_number.value_or(0), t.metadata.artist, t.metadata.title);
        } else {
          out::println(("\t\t{}. {}, {} (tombstone)"), t.metadata.track_number.value_or(0), t.metadata.artist,
                       t.metadata.title);
        }
      }
    }
  }
}

void db::set_playlists_collection_name(std::string_view name) {
  if (!collections.empty()) { collections[0].set_name(name); }
}

void db::set_loved_tracks_playlist_name(std::string_view name) {
  if (!playlists.empty()) { playlists[0].name = name; }
}

std::optional<db::track_info> db::find_track(const std::string_view& artist, const std::string_view& title,
                                             const std::string_view& collection_name,
                                             const std::string_view& playlist_name, const std::string_view& path) {
  std::unordered_set<db::track_id_t> matches;
  if (!artist.empty() && !title.empty()) {
    matches = db::track_by_artist_title(artist, title);
  } else if (auto track_id_by_path = db::track_by_path(path); track_id_by_path.has_value()) {
    matches = {track_id_by_path.value()};
  }

  if (matches.size() == 0) { return std::nullopt; }

  if (matches.size() > 1) { out::debug_warn("db::find_track(): matched multiple tracks for '{} - {}'", artist, title); }

  for (db::collection_id_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    auto& collection = db::collection_by_id(collection_id)->get();
    if (collection.name() != collection_name) { continue; }
    for (auto& playlist_id : collection.playlist_ids()) {
      auto& playlist = db::playlist_by_id(playlist_id)->get();
      if (playlist.name != playlist_name) { continue; }
      for (auto& track_id : playlist.track_ids) {
        if (matches.contains(track_id)) {
          return std::make_optional<track_info>(
            track_info{.collection_id = collection_id, .playlist_id = playlist_id, .track_id = track_id});
        }
      }
    }
  }

  return std::nullopt;
}

std::optional<std::reference_wrapper<const Collection>> db::collection_by_id(size_t id) {
  if (id >= collections.size()) { return std::nullopt; }
  return collections[id];
}

const std::vector<Collection>& db::all_collections() { return collections; }

size_t db::collection_count() { return collections.size(); }

size_t db::playlist_loved_tracks_id() { return 0; }

Playlist& db::playlist_loved_tracks() {
  if (playlists.size() == 0) {
    if (collections.size() == 0) { add_collection("Playlists"); }
    db::add_playlist_to_collection(0, db::Playlist{"Loved tracks", {""}, PlaylistType::User});
  }
  return playlists[0];
}

std::optional<size_t> db::collection_of_playlist(size_t playlist_id) {
  for (size_t collection_id = 0; collection_id < collections.size(); collection_id += 1) {
    for (size_t p : collections[collection_id].playlist_ids()) {
      if (playlist_id == p) { return collection_id; }
    }
  }
  return std::nullopt;
}

std::optional<db::collection_id_t> db::collection_of_track(db::track_id_t track_id) {
  if (collections.size() <= 1) { return std::nullopt; }
  for (size_t collection_id = 1; collection_id < collections.size(); collection_id += 1) {
    for (size_t playlist_id : collections[collection_id].playlist_ids()) {
      for (auto track_id_ : playlists[playlist_id].track_ids) {
        if (track_id_ == track_id) { return collection_id; }
      }
    }
  }
  return std::nullopt;
}

size_t db::add_collection(std::string_view name) {
  collections.emplace_back(Collection{name});
  return collections.size() - 1;
}

size_t db::add_playlist_to_collection(size_t collection_id, Playlist playlist) {
  playlists.emplace_back(std::move(playlist));
  auto& collection = collections[collection_id];
  collection.add_playlist(playlists.size() - 1);
  return playlists.size() - 1;
}

void db::mark_collection_as_tombstone(size_t collection_id) {
  if (collection_id == 0 || collection_id >= collections.size()) { return; }
  auto& collection = collections[collection_id];
  collection.set_tombstone(true);
}

bool db::add_path_to_collection(size_t collection_id, fs::path path) {
  ScopeTimer timer("add_path_to_collection");

  if (collection_id >= collections.size()) { return false; }
  auto& collection = collections[collection_id];
  if (!fs::is_directory(path) || !collection.add_path(path)) { return false; }

  return true;
}

bool db::remove_path_from_collection(size_t collection_id, fs::path path) {
  if (collection_id >= collections.size()) { return false; }
  auto& collection = collections[collection_id];
  if (!fs::is_directory(path) || !collection.remove_path(path)) { return false; }

  return true;
}

void db::rename_collection(size_t collection_id, std::string_view new_name) {
  if (collection_id >= collections.size() || collection_id == 0) { return; }
  auto& collection = collections[collection_id];
  collection.set_name(new_name);
}

std::optional<std::reference_wrapper<const Playlist>> db::playlist_by_id(size_t id) {
  if (id >= playlists.size()) { return std::nullopt; }
  return playlists[id];
}

std::optional<size_t> db::playlist_id_by_path(const fs::path& path) {
  for (size_t i = 0; i < playlists.size(); i += 1) {
    if (playlists[i].album_path == path_to_utf8(path)) { return i; }
  }
  return std::nullopt;
}

std::vector<size_t> db::playlist_ids_by_name(std::string_view n) {
  std::vector<size_t> result;
  for (size_t i = 0; i < playlists.size(); i += 1) {
    if (playlists[i].name == n) { result.emplace_back(i); }
  }
  return result;
}

const std::vector<Playlist>& db::all_playlists() { return playlists; }

size_t db::playlist_count() { return playlists.size(); }

size_t db::get_album_id(size_t collection_id, std::string album_name, std::string album_artist,
                        std::filesystem::path parent_dir_name) {
  // if album_name is empty, album name becomes the name of the parent directory of the
  // track file
  if (album_name.empty()) {
    album_name = path_to_utf8(parent_dir_name);
    album_artist = "";
  }

  // find a playlist with matching name and author
  auto& collection = collections[collection_id];
  std::vector<size_t> potential_playlist_ids = playlist_ids_by_name(album_name);
  for (size_t playlist_id : potential_playlist_ids) {
    auto& playlist = playlists[playlist_id];
    if (collection.has_playlist(playlist_id)) {
      // FIXME: we can't use album_artist to check if albums match because it's often blank on tracks
      // we fill it in later by checking the most common artist across the album's tracks
      if (album_artist.empty() || (!playlist.author.empty() && playlist.author[0] == album_artist)) {
        return playlist_id;
      }
    }
  }

  // create a new playlist if none was found
  size_t playlist_id = playlists.size();
  playlists.emplace_back(Playlist{album_name, {album_artist}, PlaylistType::Album});
  collections[collection_id].add_playlist(playlist_id);
  return playlist_id;
}

void db::mark_playlist_as_tombstone(size_t playlist_id) {
  if (playlist_id == 0 || playlist_id >= playlists.size()) { return; }
  auto& playlist = playlists[playlist_id];
  playlist.set_tombstone(true);
}

bool db::add_track_id_to_playlist(size_t playlist_id, size_t track_id) {
  if (playlist_id >= playlists.size()) { return false; }
  if (track_id >= tracks.size()) { return false; }
  auto& playlist = playlists[playlist_id];
  return playlist.add_track(track_id);
}

bool db::add_track_ids_to_playlist(size_t playlist_id, size_t at, std::span<const size_t> track_ids) {
  if (playlist_id >= playlists.size()) { return false; }

  playlists[playlist_id].insert_tracks(at, track_ids);
  return true;
}

bool db::remove_track_id_from_playlist(size_t playlist_id, size_t track_id) {
  if (playlist_id >= playlists.size()) { return false; }
  if (track_id >= tracks.size()) { return false; }
  auto& playlist = playlists[playlist_id];
  return playlist.remove_track_by_id(track_id);
}

bool db::remove_track_index_from_playlist(size_t playlist_id, size_t track_index) {
  if (playlist_id >= playlists.size()) { return false; }
  auto& playlist = playlists[playlist_id];
  return playlist.remove_track_by_index(track_index);
}

bool db::remove_track_indices_from_playlist(size_t playlist_id, std::span<const size_t> indices) {
  if (playlist_id >= playlists.size()) { return false; }

  playlists[playlist_id].remove_tracks_by_indices(indices);
  return true;
}

void db::set_playlist_image(size_t playlist_id, std::string_view image_path) {
  if (playlist_id >= playlists.size()) { return; }
  auto& playlist = playlists[playlist_id];
  // playlist.fetch_cover_art(image_path);
}

void db::reset_playlist_image(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  auto& playlist = playlists[playlist_id];
  playlist.art_64x64.clear();
  playlist.art_file_path.clear();
}

void db::rename_playlist(size_t playlist_id, std::string_view new_name) {
  if (playlist_id >= playlists.size() || playlist_id == 0) { return; }
  auto& playlist = playlists[playlist_id];
  playlist.name = new_name;
}

void db::sort_playlist_by_track_number(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  playlists[playlist_id].sort_by_track_number();
}

void db::sort_playlist_by_artist_asc(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  playlists[playlist_id].sort_by_artist_asc();
}

void db::sort_playlist_by_artist_desc(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  playlists[playlist_id].sort_by_artist_desc();
}

void db::sort_playlist_by_name_asc(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  playlists[playlist_id].sort_by_name_asc();
}

void db::sort_playlist_by_name_desc(size_t playlist_id) {
  if (playlist_id >= playlists.size()) { return; }
  playlists[playlist_id].sort_by_name_desc();
}

std::optional<std::reference_wrapper<const Track>> db::track_by_id(size_t id) {
  if (id >= tracks.size()) { return std::nullopt; }
  return tracks[id];
}

std::unordered_set<size_t> db::track_by_title(std::string_view title) {
  auto it = title_to_track_ids.find(std::string(title));
  if (it == title_to_track_ids.end()) { return {}; }
  return it->second;
}

std::unordered_set<db::track_id_t> db::track_by_file_name(const std::filesystem::path& file_name) {
  auto it = file_name_to_track_ids.find(file_name);
  if (it == file_name_to_track_ids.end()) { return {}; }
  return it->second;
}

std::unordered_set<size_t> db::track_by_artist_title(std::string_view artist, std::string_view title) {
  // ensure(tracks.size() == title_to_track_ids.size());
  auto it = title_to_track_ids.find(std::string(title));
  if (it == title_to_track_ids.end()) { return {}; }
  std::unordered_set<size_t> res;
  for (auto& track_id : it->second) {
    if (tracks[track_id].metadata.artist == artist) { res.emplace(track_id); }
  }
  return res;
}

std::optional<size_t> db::track_by_path(const std::filesystem::path& path) {
  // ensure(tracks.size() == path_to_track_id.size());
  auto it = path_to_track_id.find(std::string(path));
  if (it == path_to_track_id.end()) { return std::nullopt; }
  return it->second;
}

const std::vector<Track>& db::all_tracks() { return tracks; }

size_t db::track_count() { return tracks.size(); }

db::track_id_t db::add_orphaned_track(db::collection_id_t collection_id, db::Track track_) {
  tracks.emplace_back(std::move(track_));
  tracks.back().set_flag(NEW);
  tracks.back().set_flag(ORPHANED);
  orphaned_tracks.emplace_back(collection_id, tracks.size() - 1);

  auto& track = tracks.back();
  path_to_track_id[track.file_path] = tracks.size() - 1;
  title_to_track_ids[track.metadata.title].insert(tracks.size() - 1);
  file_name_to_track_ids[track.file_name_without_extension()].insert(tracks.size() - 1);

  return tracks.size() - 1;
}

void assign_album_authors(Playlist& playlist) {
  auto playlist_author = playlist.author_pretty();
  if (playlist_author.empty() && playlist.get_tracks_count() > 0) {
    std::unordered_map<std::string_view, i32> artist_counts;
    for (size_t track_id : playlist.get_track_ids()) {
      auto& track = tracks[track_id];
      if (!track.metadata.artist.empty()) { artist_counts[track.metadata.artist] += 1; }
      for (auto& artist : track.metadata.album_artist) {
        if (artist.empty()) { continue; }
        artist_counts[artist] += 1;
      }
    }

    if (!artist_counts.empty()) {
      std::vector<std::pair<std::string_view, i32>> sorted_artists(artist_counts.begin(), artist_counts.end());

      std::sort(sorted_artists.begin(), sorted_artists.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

      for (auto& [artist, count] : sorted_artists) {
        float ratio = count / (float)playlist.get_tracks_count();
        if (ratio >= 0.5) { playlist.author.emplace_back(artist); }
      }
    }
  }
}

void setup_albums(db::playlist_id_t playlist_id) {
  auto& playlist = playlists[playlist_id];

  if (playlist.type == db::PlaylistType::Album) { playlist.sort_by_track_number(); }

  // mark tracks not found during rescan as tombstone
  for (size_t track_id : playlist.get_track_ids()) {
    auto& track = tracks[track_id];
    if (track.get_flag(db::NOT_FOUND_DURING_RESCAN)) { track.set_flag(db::TOMBSTONE); }
  }

  assign_album_authors(playlist);
}

void db::assign_orphaned_tracks() {

  std::unordered_set<playlist_id_t> album_ids;

  for (auto [collection_id, track_id] : orphaned_tracks) {
    auto& track = tracks[track_id];
    track.unset_flag(ORPHANED);
    std::string album_artist = track.pretty_album_artist();
    auto album_id = db::get_album_id(collection_id, track.metadata.album, album_artist, track.parent_directory_name());
    add_track_id_to_playlist(album_id, track_id);
    tracks[track_id].originating_album_id = album_id;
    album_ids.insert(album_id);
  }

  for (auto album_id : album_ids) {
    setup_albums(album_id);
  }

  orphaned_tracks.clear();
}

void db::set_track_playback_error(size_t track_id, bool error) {
  if (track_id >= tracks.size()) { return; }
  auto& track = tracks[track_id];
  if (error) {
    track.set_flag(PLAYBACK_ERROR);
  } else {
    track.unset_flag(PLAYBACK_ERROR);
  }
}

void db::mark_track_as_tombstone(track_id_t track_id) { tracks[track_id].set_flag(TOMBSTONE); }

void db::set_track_flag(track_id_t track_id, TrackFlag track_flag, bool state) {
  if (track_id >= tracks.size()) { return; }
  if (state) {
    tracks[track_id].set_flag(track_flag);
  } else {
    tracks[track_id].unset_flag(track_flag);
  }
}

void db::set_track_file_size(track_id_t track_id, u64 file_size) {
  if (track_id >= tracks.size()) { return; }
  tracks[track_id].file_size = file_size;
}

void db::set_track_last_modified(track_id_t track_id, i64 last_modified) {
  if (track_id >= tracks.size()) { return; }
  tracks[track_id].last_modified = last_modified;
}

void db::set_track_file_path(track_id_t track_id, fs::path file_path) {
  if (track_id >= tracks.size()) { return; }
  auto& track = tracks[track_id];
  if (file_path == track.file_path) { return; }
  auto old_file_name = track.file_name_without_extension();
  track.file_path = file_path;
  auto new_file_name = track.file_name_without_extension();
  if (old_file_name != new_file_name) {
    file_name_to_track_ids[old_file_name].erase(track_id);
    file_name_to_track_ids[new_file_name].insert(track_id);
  }
}
void db::set_track_metadata(track_id_t track_id, zincbox::TrackMetadata metadata) {
  if (track_id >= tracks.size()) { return; }
  tracks[track_id].metadata = std::move(metadata);
}

std::vector<db::playlist_info> db::search_playlists(std::string_view search_text, size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);

  std::vector<playlist_info> result;
  for (size_t collection_id = 0; collection_id < collections.size(); collection_id += 1) {
    auto& collection = db::collection_by_id(collection_id)->get();
    if (collection.is_tombstone()) { continue; }
    for (size_t playlist_id : collection.playlist_ids()) {
      auto& playlist = db::playlist_by_id(playlist_id)->get();
      if (playlist.is_tombstone()) { continue; }

      bool pass = false;
      auto playlist_name_sanitized = sanitize_query(playlist.name);
      if (playlist_name_sanitized.contains(query_sanitized)) { pass = true; }
      bool playlist_author_contains = false;
      for (auto& playlist_author : playlist.author) {
        if (sanitize_query(playlist_author).contains(query_sanitized)) {
          playlist_author_contains = true;
          break;
        }
      }
      if (!pass && playlist_author_contains) { pass = true; }
      if (!pass) { continue; }

      result.emplace_back(playlist_info{.collection_id = collection_id, .playlist_id = playlist_id});
      if (result.size() >= max_size) { break; }
    }
  }
  return result;
}

std::vector<db::playlist_info> db::search_playlists(std::string_view search_text, size_t collection_id,
                                                    size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);

  std::vector<playlist_info> result;
  for (size_t playlist_id : collections[collection_id].playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }

    bool pass = false;
    auto playlist_name_sanitized = sanitize_query(playlist.name);
    if (playlist_name_sanitized.contains(query_sanitized)) { pass = true; }
    bool playlist_author_contains = false;
    for (auto& playlist_author : playlist.author) {
      if (sanitize_query(playlist_author).contains(query_sanitized)) {
        playlist_author_contains = true;
        break;
      }
    }
    if (!pass && playlist_author_contains) { pass = true; }
    if (!pass) { continue; }

    result.emplace_back(playlist_info{.collection_id = collection_id, .playlist_id = playlist_id});
    if (result.size() >= max_size) { break; }
  }
  return result;
}

std::vector<size_t> db::search_playlists(std::string_view search_text, std::span<size_t> playlist_ids,
                                         size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);

  std::vector<size_t> result;
  for (size_t playlist_id : playlist_ids) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }

    bool pass = false;
    auto playlist_name_sanitized = sanitize_query(playlist.name);
    if (playlist_name_sanitized.contains(query_sanitized)) { pass = true; }
    bool playlist_author_contains = false;
    for (auto& playlist_author : playlist.author) {
      if (sanitize_query(playlist_author).contains(query_sanitized)) {
        playlist_author_contains = true;
        break;
      }
    }
    if (!pass && playlist_author_contains) { pass = true; }
    if (!pass) { continue; }

    result.emplace_back(playlist_id);
    if (result.size() >= max_size) { break; }
  }
  return result;
}

std::vector<db::track_info> db::search_tracks(std::string_view search_text, size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);
  std::vector<db::track_info> result;

  for (size_t collection_id = 0; collection_id < collections.size(); collection_id += 1) {
    auto found_tracks = search_tracks(query_sanitized, collection_id, max_size - result.size());
    result.insert(result.end(), found_tracks.begin(), found_tracks.end());
    if (result.size() >= max_size) {
      result.resize(max_size);
      return result;
    }
  }
  return result;
}

bool sanitize_and_contains(const std::string& query_sanitized, const std::string& field) {
  auto s = sanitize_query(field);
  return s.contains(query_sanitized);
};

std::vector<db::track_info> db::search_tracks(std::string_view search_text, size_t collection_id, size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);
  std::vector<db::track_info> result;
  if (collections[collection_id].is_tombstone()) { return {}; }

  for (size_t playlist_id : collections[collection_id].playlist_ids()) {
    if (playlists[playlist_id].is_tombstone()) { continue; }
    for (size_t track_id : playlists[playlist_id].track_ids) {
      auto& track = db::track_by_id(track_id)->get();
      if (track.get_flag(TOMBSTONE)) { continue; }

      bool album_artist_contains = false;
      for (auto& a : track.metadata.album_artist) {
        if (sanitize_and_contains(query_sanitized, a)) {
          album_artist_contains = true;
          break;
        }
      }

      if (sanitize_and_contains(query_sanitized, track.metadata.title) ||
          sanitize_and_contains(query_sanitized, track.metadata.artist) || album_artist_contains ||
          sanitize_and_contains(query_sanitized, path_to_utf8(track.file_name_without_extension()))) {

        result.emplace_back(db::track_info{collection_id, playlist_id, track_id});
        if (result.size() >= max_size) { return result; }
      }
    }
  }
  return result;
}

std::vector<db::track_info> db::search_tracks(std::string_view search_text, std::span<track_info> src,
                                              size_t max_size) {
  auto query_sanitized = sanitize_query(search_text);
  std::vector<db::track_info> result;
  for (db::track_info track_info_ : src) {
    auto& track = db::track_by_id(track_info_.track_id)->get();
    if (track.get_flag(TOMBSTONE)) { continue; }

    bool album_artist_contains = false;
    for (auto& a : track.metadata.album_artist) {
      if (sanitize_and_contains(query_sanitized, a)) {
        album_artist_contains = true;
        break;
      }
    }

    if (sanitize_and_contains(query_sanitized, track.metadata.title) ||
        sanitize_and_contains(query_sanitized, track.metadata.artist) || album_artist_contains ||
        sanitize_and_contains(query_sanitized, path_to_utf8(track.file_name_without_extension()))) {

      result.emplace_back(track_info_);
      if (result.size() >= max_size) { break; }

    } else {
      continue;
    }
  }
  return result;
}
