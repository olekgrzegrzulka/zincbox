#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "common/debug.hpp"
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "core/io.hpp"
#include "core/musicdb.hpp"
#include "core/utf.hpp"

using namespace db;

static std::vector<Collection> collections;
static std::vector<Playlist> playlists;
static std::vector<Track> tracks;

void db::mark_collection_as_tombstone(size_t collection_id) {
  auto& collection = collection_by_id(collection_id)->get();
  collection.set_tombstone(true);

  for (size_t playlist_id : collection.playlist_ids) {
    mark_playlist_as_tombstone(playlist_id);
  }
}
void db::mark_playlist_as_tombstone(size_t playlist_id) {
  auto& playlist = db::playlist_by_id(playlist_id)->get();
  // Only mark tracks as tombstone for PlaylistType::Album,
  // PlaylistType::User and PlaylistType::Smart don't own the tracks themselves
  if (playlist.type == db::PlaylistType::Album) {
    playlist.set_tombstone(true);

    for (size_t track_id : playlist.get_track_ids()) {
      auto& track = tracks[track_id];
      track.set_tombstone(true);
    }
  } else if (playlist.type == db::PlaylistType::User) {
    playlist.set_tombstone(true);
  } else if (playlist.type == db::PlaylistType::Smart) {
    playlist.set_tombstone(true);
  } else {
    debug_warn("db::mark_playlist_as_tombstone(): unknown db::PlaylistType enum value");
  }
}

size_t db::add_collection(std::u32string_view name) {
  collections.emplace_back(Collection{name});
  return collections.size() - 1;
}

std::optional<std::reference_wrapper<Collection>> db::collection_by_id(size_t id) {
  if (id >= collections.size()) { return std::nullopt; }
  return collections[id];
}

std::optional<std::reference_wrapper<const Track>> db::track_by_id(size_t id) {
  if (id >= tracks.size()) { return std::nullopt; }
  return tracks[id];
}

std::optional<size_t> db::track_by_title(std::u32string_view title) {
  for (size_t i = 0; i < tracks.size(); i += 1) {
    if (tracks[i].title == title) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<Playlist>> db::playlist_by_id(size_t id) {
  if (id >= playlists.size()) { return std::nullopt; }
  return playlists[id];
}

std::optional<std::reference_wrapper<Playlist>> db::playlist_by_name(std::u32string_view n) {
  for (auto& p : playlists) {
    if (p.name == n) { return p; }
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<Playlist>> db::playlist_by_path(fs::path path) {
  for (auto& p : playlists) {
    if (p.album_path == utf8_to_utf32(path.string())) { return p; }
  }
  return std::nullopt;
}

const std::vector<Playlist>& db::all_playlists() { return playlists; }

const std::vector<Collection>& db::all_collections() { return collections; }
const std::vector<Track>& db::all_tracks() { return tracks; }
size_t db::collection_count() { return collections.size(); }
size_t db::playlist_loved_tracks_id() { return 0; }
Playlist& db::playlist_loved_tracks() {
  if (playlists.size() == 0) {
    if (collections.size() == 0) { add_collection(U"Playlists"); }
    collections[0].add_playlist(U"Loved tracks", U"");
  }
  return playlists[0];
}
size_t db::playlist_count() { return playlists.size(); }
size_t db::track_count() { return tracks.size(); }

size_t db::add_track(i32 track_number,
                     std::u32string title,
                     std::u32string artist,
                     std::u32string album_artist,
                     std::u32string genre,
                     i32 year,
                     i32 bitrate,
                     i32 length_seconds,
                     std::u32string path) {

  Track t{};
  t.track_number = track_number;
  t.title = title;
  t.artist = artist;
  t.album_artist = album_artist;
  t.genre = genre;
  t.year = year;
  t.bitrate = bitrate;
  t.length_seconds = length_seconds;
  t.path = path;
  tracks.emplace_back(t);
  return tracks.size() - 1;
}

size_t db::add_track(std::ifstream& is) {
  Track t{is};
  tracks.emplace_back(t);
  return tracks.size() - 1;
}

Track::Track(std::ifstream& is) {
  read_bin(is, track_number);
  read_str(is, title);
  read_str(is, artist);
  read_str(is, album_artist);
  read_str(is, genre);
  read_bin(is, year);
  read_bin(is, bitrate);
  read_bin(is, length_seconds);
  read_str(is, path);
}

void Track::set_tombstone(bool t) {
  tombstone = t;
}

void Track::set_orphan(bool o) {
  orphan = o;
}

void Track::serialize(std::ostream& os) {
  write_bin(os, track_number);
  write_str(os, title);
  write_str(os, artist);
  write_str(os, album_artist);
  write_str(os, genre);
  write_bin(os, year);
  write_bin(os, bitrate);
  write_bin(os, length_seconds);
  write_str(os, path);
}

bool Collection::add_path(fs::path path, bool internal) {
  bool has_path = false;
  if (!internal) {
    if (paths.contains(path)) {
      has_path = true;
    } else {
      paths.emplace(path);
    }
  } else {
    if (paths_internal.contains(path)) {
      has_path = true;
    } else {
      paths_internal.emplace(path);
    }
  }

  db::Playlist* playlist = nullptr;
  if (has_path) {
    auto p = playlist_by_path(path);
    if (p.has_value()) {
      playlist = &p->get();
      (*playlist) = Playlist{utf8_to_utf32(path.filename().string()), U"", PlaylistType::Album};
    }
  }
  if (!playlist) {
    size_t playlist_i = add_album(utf8_to_utf32(path.filename().string()), U"");
    playlist = &playlist_by_id(playlist_i)->get();
  }
  ensure(playlist);
  playlist->album_path = utf8_to_utf32(path.string());

  for (const auto& entry : fs::directory_iterator(path)) {
    if (entry.is_directory()) {
      add_path(entry.path(), true);
    } else if (entry.is_regular_file()) {
      if (io::is_cover_file(entry.path())) {
        io::add_cover_file(*playlist, entry.path());
        debug_log("added cover file: ", entry.path().string());
      } else if (io::is_music_file(entry.path())) {
        io::add_music_file(*playlist, entry.path());
        debug_log("added music file: ", entry.path().string());
      } else {
        debug_warn("unknown file type: ", entry.path().string());
      }
    }
  }

  return true;
}

bool Playlist::add_track(size_t track_id) {
  if (track_id >= db::track_count() || has_track_id(track_id)) {
    return false;
  }
  track_ids.emplace_back(track_id);
  return true;
}

bool Playlist::remove_track_by_id(size_t track_id) {
  auto index = find_track_index(track_id);
  if (!index.has_value()) { return false; }
  track_ids.erase(track_ids.begin() + index.value());
  return true;
}

bool Playlist::remove_track_by_index(size_t index) {
  if (index >= track_ids.size()) { return false; }
  track_ids.erase(track_ids.begin() + index);
  return true;
}

void Playlist::sort() {
  std::sort(track_ids.begin(), track_ids.end(), [&](size_t lhs, size_t rhs) {
    return tracks[lhs].track_number < tracks[rhs].track_number;
  });
}

void Playlist::set_tombstone(bool t) {
  tombstone = t;
}

std::optional<size_t> Playlist::next_track_id(size_t track_id) {
  auto index = find_track_index(track_id);
  if (index.has_value() && index.value() + 1 < track_ids.size()) {
    return track_ids[index.value() + 1];
  } else {
    return std::nullopt;
  }
}

std::optional<size_t> Playlist::prev_track_id(size_t track_id) {
  auto index = find_track_index(track_id);
  if (index.has_value() && index.value() > 0) {
    return track_ids[index.value() - 1];
  } else {
    return std::nullopt;
  }
}

bool Playlist::has_track_id(size_t track_id) {
  auto index = find_track_index(track_id);
  if (index.has_value()) {
    return true;
  } else {
    return false;
  }
}

std::optional<size_t> Playlist::find_track_index(size_t track_id) {
  for (size_t i = 0; i < track_ids.size(); i += 1) {
    if (track_ids[i] == track_id) {
      return i;
    }
  }
  return std::nullopt;
}

void Playlist::serialize(std::ostream& os) {
  write_str(os, name);
  write_str(os, author);
  write_bin(os, image.size());
  for (auto a : image) {
    write_bin(os, a);
  }
  write_bin(os, type);
  write_bin(os, track_ids.size());
  for (size_t track_id : track_ids) {
    write_bin(os, track_id);
  }
}

void Playlist::serialize(std::ostream& os, const std::unordered_map<size_t, size_t>& old_track_id_to_new_track_id) {
  write_str(os, name);
  write_str(os, author);
  write_bin(os, image.size());
  for (auto a : image) {
    write_bin(os, a);
  }
  write_bin(os, type);

  size_t new_track_ids_size = 0;
  for (size_t track_id : track_ids) {
    if (old_track_id_to_new_track_id.contains(track_id)) {
      new_track_ids_size += 1;
    }
  }
  write_bin(os, new_track_ids_size);
  for (size_t track_id : track_ids) {
    if (old_track_id_to_new_track_id.contains(track_id)) {
      write_bin(os, old_track_id_to_new_track_id.at(track_id));
    }
  }
}

size_t Collection::add_album(std::u32string_view title, std::u32string_view artist) {
  Playlist a{title, artist, PlaylistType::Album};
  playlists.emplace_back(a);
  playlist_ids.emplace_back(playlists.size() - 1);
  return playlists.size() - 1;
}

size_t Collection::add_playlist(std::u32string_view title, std::u32string_view artist) {
  Playlist a{title, artist, PlaylistType::User};
  playlists.emplace_back(a);
  playlist_ids.emplace_back(playlists.size() - 1);
  return playlists.size() - 1;
}

Playlist::Playlist(std::ifstream& is) {
  read_str(is, name);
  read_str(is, author);
  size_t image_size = 0;
  read_bin(is, image_size);
  image.resize(image_size);
  for (size_t i = 0; i < image_size; i += 1) {
    u8 value;
    read_bin(is, value);
    image[i] = value;
  }
  read_bin(is, type);
  size_t track_ids_size = 0;
  read_bin(is, track_ids_size);
  track_ids.resize(track_ids_size);
  for (size_t i = 0; i < track_ids_size; i += 1) {
    size_t value;
    read_bin(is, value);
    track_ids[i] = value;
  }
}

size_t Collection::add_playlist(std::ifstream& is) {
  Playlist playlist{is};
  playlists.emplace_back(playlist);
  playlist_ids.emplace_back(playlists.size() - 1);
  return playlists.size() - 1;
}

std::optional<size_t> Collection::next_playlist_id(size_t playlist_id) {
  auto index = find_playlist_index(playlist_id);
  if (index.has_value() && index.value() + 1 < playlist_ids.size()) {
    return playlist_ids[index.value() + 1];
  } else {
    return std::nullopt;
  }
}

std::optional<size_t> Collection::prev_playlist_id(size_t playlist_id) {
  auto index = find_playlist_index(playlist_id);
  if (index.has_value() && index.value() > 0) {
    return playlist_ids[index.value() - 1];
  } else {
    return std::nullopt;
  }
}

void Collection::set_tombstone(bool t) {
  tombstone = t;
}

Collection::Collection(std::ifstream& is) {
  read_str(is, name);
  size_t playlist_ids_size = 0;
  read_bin(is, playlist_ids_size);
  playlist_ids.resize(playlist_ids_size);
  for (size_t i = 0; i < playlist_ids_size; i += 1) {
    size_t value;
    read_bin(is, value);
    playlist_ids[i] = value;
  }
}

void Collection::serialize(std::ofstream& os) {
  write_str(os, name);
  write_bin(os, playlist_ids.size());
  for (size_t playlist_id : playlist_ids) {
    write_bin(os, playlist_id);
  }
}

void Collection::serialize(std::ofstream& os, const std::unordered_map<size_t, size_t>& old_playlist_id_to_new_playlist_id) {
  write_str(os, name);
  size_t new_playlist_ids_size = 0;
  for (size_t playlist_id : playlist_ids) {
    if (old_playlist_id_to_new_playlist_id.contains(playlist_id)) {
      new_playlist_ids_size += 1;
    }
  }
  write_bin(os, new_playlist_ids_size);
  for (size_t playlist_id : playlist_ids) {
    if (old_playlist_id_to_new_playlist_id.contains(playlist_id)) {
      write_bin(os, old_playlist_id_to_new_playlist_id.at(playlist_id));
    }
  }
}

std::optional<size_t> Collection::find_playlist_index(size_t playlist_id) {
  for (size_t i = 0; i < playlist_ids.size(); i += 1) {
    if (playlist_ids[i] == playlist_id) {
      return i;
    }
  }
  return std::nullopt;
}

void db::serialize(std::ofstream& os) {
  for (size_t track_id = 0; track_id < tracks.size(); track_id += 1) {
    tracks[track_id].set_orphan(true);
  }

  // Unorphan tracks present in some non-tombstoned playlist/album
  for (size_t collection_id = 0; collection_id < collections.size(); collection_id += 1) {
    if (collections[collection_id].is_tombstone()) { continue; }

    for (size_t playlist_id : collections[collection_id].playlist_ids) {
      auto& playlist = playlist_by_id(playlist_id)->get();
      if (playlist.is_tombstone()) { continue; }

      for (size_t track_id : playlist.get_track_ids()) {
        tracks[track_id].set_orphan(false);
      }
    }
  }

  std::unordered_map<size_t, size_t> old_track_id_to_new_track_id;

  size_t old_track_id = 0;
  size_t nonorphaned_tracks_count = 0;
  for (auto& t : tracks) {
    if (!t.is_orphan()) {
      old_track_id_to_new_track_id[old_track_id] = nonorphaned_tracks_count;
      nonorphaned_tracks_count += 1;
    }
    old_track_id += 1;
  }

  std::unordered_map<size_t, size_t> old_playlist_id_to_new_playlist_id;
  size_t old_playlist_id = 0;
  size_t nontombstoned_playlists_count = 0;
  for (auto& p : playlists) {
    if (!p.is_tombstone()) {
      old_playlist_id_to_new_playlist_id[old_playlist_id] = nontombstoned_playlists_count;
      nontombstoned_playlists_count += 1;
    }
    old_playlist_id += 1;
  }

  size_t nontombstoned_collections_count = 0;
  for (auto& c : collections) {
    if (!c.is_tombstone()) {
      nontombstoned_collections_count += 1;
    }
  }

  write_bin(os, nontombstoned_collections_count);
  for (auto& c : collections) {
    if (!c.is_tombstone()) {
      c.serialize(os, old_playlist_id_to_new_playlist_id);
    }
  }

  write_bin(os, nontombstoned_playlists_count);
  for (auto& p : playlists) {
    if (!p.is_tombstone()) {
      p.serialize(os, old_track_id_to_new_track_id);
    }
  }

  write_bin(os, nonorphaned_tracks_count);
  for (auto& t : tracks) {
    if (!t.is_orphan()) {
      t.serialize(os);
    }
  }
}

void db::deserialize(std::ifstream& is) {
  collections.clear();
  playlists.clear();
  tracks.clear();

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
    tracks.emplace_back(Track{is});
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
