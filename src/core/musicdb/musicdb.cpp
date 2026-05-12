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
#include "collection.hpp"
#include "common/debug.hpp"
#include "common/search_utils.hpp"
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "core/io.hpp"
#include "core/utf.hpp"
#include "musicdb.hpp"
#include "playlist.hpp"
#include "track.hpp"

using namespace db;

static std::vector<db::Collection> collections;
static std::vector<db::Playlist> playlists;
static std::vector<db::Track> tracks;
static std::unordered_map<std::u32string, size_t> path_to_track_id;
static std::unordered_map<std::u32string, std::unordered_set<size_t>> title_to_track_ids;

void db::create_empty_db() {
  collections.clear();
  playlists.clear();
  tracks.clear();
  path_to_track_id.clear();
  title_to_track_ids.clear();
  db::add_collection(U"Playlists");
  db::add_playlist_to_collection(0, db::Playlist{U"Loved tracks", U"", db::PlaylistType::User});
}

void db::serialize(std::ofstream& os) {
  // Initially mark all tracks and playlists as tombstone, and keep track of playlists removed by user
  for (size_t track_id = 0; track_id < tracks.size(); track_id += 1) {
    tracks[track_id].set_tombstone(true);
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
    for (size_t playlist_id : collections[collection_id].playlist_ids) {

      // Check if playlist was removed by user, and if so skip it (keep it tombstoned)
      if (playlist_ids_removed_by_user_or_empty.contains(playlist_id)) { continue; }

      auto& playlist = playlists[playlist_id];
      playlist.set_tombstone(false);
      for (size_t track_id : playlist.get_track_ids()) {
        tracks[track_id].set_tombstone(false);
      }
    }
  }

  std::vector<size_t> old_track_id_to_new_track_id(tracks.size());
  size_t nontombstoned_tracks_count = 0;
  for (size_t old_track_id = 0; old_track_id < tracks.size(); old_track_id += 1) {
    if (!tracks[old_track_id].is_tombstone()) {
      old_track_id_to_new_track_id[old_track_id] = nontombstoned_tracks_count;
      nontombstoned_tracks_count += 1;
    }
  }

  std::vector<size_t> old_playlist_id_to_new_playlist_id(playlists.size(), playlists.size());
  size_t nontombstoned_playlists_count = 0;
  for (size_t old_playlist_id = 0; old_playlist_id < playlists.size(); old_playlist_id += 1) {
    if (!playlists[old_playlist_id].is_tombstone()) {
      old_playlist_id_to_new_playlist_id[old_playlist_id] = nontombstoned_playlists_count;
      nontombstoned_playlists_count += 1;
    }
  }

  size_t nontombstoned_collections_count = 0;
  for (size_t old_collection_id = 0; old_collection_id < collections.size(); old_collection_id += 1) {
    if (!collections[old_collection_id].is_tombstone()) {
      nontombstoned_collections_count += 1;
    }
  }

  write_bin(os, nontombstoned_collections_count);
  for (auto& c : collections) {
    if (c.is_tombstone()) { continue; }
    c.serialize(os);
  }

  write_bin(os, nontombstoned_playlists_count);
  for (auto& p : playlists) {
    if (p.is_tombstone()) { continue; }
    p.serialize(os, old_track_id_to_new_track_id);
  }

  write_bin(os, nontombstoned_tracks_count);
  for (auto& t : tracks) {
    if (t.is_tombstone()) { continue; }
    t.serialize(os);
  }
}

void db::deserialize(std::ifstream& is) {
  collections.clear();
  playlists.clear();
  tracks.clear();
  path_to_track_id.clear();
  title_to_track_ids.clear();

  size_t collections_size = 0;
  read_bin(is, collections_size);
  for (size_t i = 0; i < collections_size; i += 1) {
    collections.emplace_back(Collection{is});
  }

  size_t playlists_size = 0;
  read_bin(is, playlists_size);
  for (size_t i = 0; i < playlists_size; i += 1) {
    playlists.emplace_back(Playlist{is});
  }

  size_t tracks_size = 0;
  read_bin(is, tracks_size);
  for (size_t i = 0; i < tracks_size; i += 1) {
    Track t{is};
    tracks.emplace_back(t);
    if (!title_to_track_ids.contains(t.title)) {
      title_to_track_ids[t.title] = {tracks.size() - 1};
    } else {
      title_to_track_ids[t.title].insert(tracks.size() - 1);
    }
    path_to_track_id[t.path] = tracks.size() - 1;
  }

  for (auto& p : playlists) {
    if (p.type == PlaylistType::Album) {
      p.sort_by_track_number();
    }
  }
}

void db::print_collections() {
  for (auto& c : collections) {
    debug_log("COLLECTION ", utf32_to_utf8(c.name));
    for (auto p_id : c.playlist_ids) {
      auto& p = playlists[p_id];
      debug_log("\tPlaylist ", utf32_to_utf8(p.name), " - ", utf32_to_utf8(p.author));
      for (size_t t_id : p.get_track_ids()) {
        auto& t = tracks[t_id];
        debug_log("\t\t", t.track_number, ". ", utf32_to_utf8(t.artist), " - ", utf32_to_utf8(t.title));
      }
    }
  }
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
    if (collections.size() == 0) { add_collection(U"Playlists"); }
    db::add_playlist_to_collection(0, db::Playlist{U"Loved tracks", U"", PlaylistType::User});
  }
  return playlists[0];
}

size_t db::add_collection(std::u32string_view name) {
  collections.emplace_back(Collection{name});
  return collections.size() - 1;
}

size_t db::add_playlist_to_collection(size_t collection_id, Playlist playlist) {
  playlists.emplace_back(playlist);
  auto& collection = collections[collection_id];
  collection.add_playlist(playlists.size() - 1);
  return playlists.size() - 1;
}

void db::mark_collection_as_tombstone(size_t collection_id) {
  if (collection_id == 0 || collection_id >= collections.size()) { return; }
  auto& collection = collections[collection_id];
  collection.set_tombstone(true);
}

void visit_directory(size_t collection_id, std::string_view path) {
  std::unordered_set<size_t> album_ids_visited;
  std::optional<fs::path> cover_file_path;

  for (auto& entry : fs::directory_iterator(path)) {
    if (entry.is_directory()) {
      visit_directory(collection_id, entry.path().string());
    } else if (entry.is_regular_file()) {
      if (io::is_music_file(entry)) {
        io::TrackFile track(entry.path());
        if (track.is_valid()) {
          size_t playlist_id = get_album_id(collection_id, track.get_album_name(), track.get_album_artist());
          album_ids_visited.insert(playlist_id);
          add_track_to_playlist(playlist_id, track.get_track().value());
          if (playlists[playlist_id].image.empty()) {
            playlists[playlist_id].image = track.get_album_art();
          }
        } else {
        }
      } else if (io::is_cover_file(entry) && !cover_file_path.has_value()) {
        cover_file_path = entry.path();
      }
    }
  }

  if (cover_file_path.has_value()) {
    for (size_t playlist_id : album_ids_visited) {
      auto& playlist = playlists[playlist_id];
      if (playlist.image.empty()) {
        io::add_cover_file(playlist, *cover_file_path);
      }
    }
  }
}

bool db::add_path_to_collection(size_t collection_id, std::string_view path) {
  ScopeTimer timer("add_path_to_collection");

  if (collection_id >= collections.size()) { return false; }
  auto& collection = collections[collection_id];
  if (!collection.add_path(path)) { return false; }

  visit_directory(collection_id, path);

  for (size_t playlist_id : collection.playlist_ids) {
    auto& playlist = playlists[playlist_id];
    if (playlist.type == PlaylistType::Album) {
      playlist.sort_by_track_number();
    }
  }

  return true;
}

void db::rescan_collection(size_t collection_id) {
  debug_warn("db::rescan_collection: TODO");
  if (collection_id >= collections.size()) { return; }
  auto& collection = collections[collection_id];
}

std::optional<std::reference_wrapper<const Playlist>> db::playlist_by_id(size_t id) {
  if (id >= playlists.size()) { return std::nullopt; }
  return playlists[id];
}

std::optional<size_t> db::playlist_id_by_path(fs::path path) {
  for (size_t i = 0; i < playlists.size(); i += 1) {
    if (playlists[i].album_path == utf8_to_utf32(path.string())) { return i; }
  }
  return std::nullopt;
}

std::vector<size_t> db::playlist_ids_by_name(std::u32string_view n) {
  std::vector<size_t> result;
  for (size_t i = 0; i < playlists.size(); i += 1) {
    if (playlists[i].name == n) { result.emplace_back(i); }
  }
  return result;
}

const std::vector<Playlist>& db::all_playlists() { return playlists; }

size_t db::playlist_count() { return playlists.size(); }

size_t db::get_album_id(size_t collection_id, std::u32string_view album_name, std::u32string_view album_artist) {
  // find a playlist with matching name and author
  auto& collection = collections[collection_id];
  std::vector<size_t> potential_playlist_ids = playlist_ids_by_name(album_name);
  for (size_t playlist_id : potential_playlist_ids) {
    auto& playlist = playlists[playlist_id];
    if (collection.has_playlist(playlist_id) && playlist.author == album_artist) { return playlist_id; }
  }

  // create a new playlist if none was found
  size_t playlist_id = playlists.size();
  playlists.emplace_back(Playlist{album_name, album_artist, PlaylistType::Album});
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

size_t db::add_track_to_playlist(size_t playlist_id, Track track) {
  ScopeTimer timer("add_track_to_playlist");
  // check if track with the same file path is already present in the db
  if (auto found_track = db::track_by_path(track.path); found_track != std::nullopt) {
    // print both paths
    playlists[playlist_id].add_track(*found_track);
    return *found_track;
  }

  auto compare_metadata = [](auto a, auto b) {
    if constexpr (std::is_arithmetic_v<std::decay_t<decltype(a)>> && std::is_arithmetic_v<std::decay_t<decltype(b)>>) {
      return a == b;
    } else {
      return sanitize_query(a) == sanitize_query(b);
    }
  };

  // check if track with similar metadata is already present in the db
  std::map<i32, size_t> track_similiarity;
  if (auto found_tracks = db::track_by_title(track.title); found_tracks.size() > 0) {
    for (size_t found_track_id : found_tracks) {
      i32 similiarity_index = 0;
      similiarity_index += 1 * compare_metadata(db::track_by_id(found_track_id)->get().track_number, track.track_number);
      similiarity_index += 3 * compare_metadata(db::track_by_id(found_track_id)->get().artist, track.artist);
      similiarity_index += 1 * compare_metadata(db::track_by_id(found_track_id)->get().album_artist, track.album_artist);
      similiarity_index += 1 * compare_metadata(db::track_by_id(found_track_id)->get().genre, track.genre);
      similiarity_index += 1 * compare_metadata(db::track_by_id(found_track_id)->get().year, track.year);
      track_similiarity[similiarity_index] = found_track_id;
      if (similiarity_index > 0) {
        playlists[playlist_id].add_track(found_track_id);
      }
    }
  }

  if (track_similiarity.size() > 0) {
    if (track_similiarity.rbegin()->first > 3) {
      size_t found_track_id = track_similiarity.rbegin()->second;
      playlists[playlist_id].add_track(found_track_id);
      return found_track_id;
    }
  }

  if (!title_to_track_ids.contains(track.title)) {
    title_to_track_ids[track.title] = {tracks.size()};
  } else {
    title_to_track_ids[track.title].insert(tracks.size());
  }
  path_to_track_id[track.path] = tracks.size();
  playlists[playlist_id].add_track(tracks.size());

  tracks.emplace_back(track);
  return tracks.size() - 1;
}

std::optional<std::reference_wrapper<const Track>> db::track_by_id(size_t id) {
  if (id >= tracks.size()) { return std::nullopt; }
  return tracks[id];
}

std::unordered_set<size_t> db::track_by_title(std::u32string_view title) {
  // ensure(tracks.size() == title_to_track_ids.size());
  auto it = title_to_track_ids.find(std::u32string(title));
  if (it == title_to_track_ids.end()) { return {}; }
  return it->second;
}

std::optional<size_t> db::track_by_path(std::u32string_view path) {
  // ensure(tracks.size() == path_to_track_id.size());
  auto it = path_to_track_id.find(std::u32string(path));
  if (it == path_to_track_id.end()) { return std::nullopt; }
  return it->second;
}

const std::vector<Track>& db::all_tracks() { return tracks; }

size_t db::track_count() { return tracks.size(); }
