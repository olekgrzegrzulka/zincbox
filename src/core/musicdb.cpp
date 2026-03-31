#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
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

bool Collection::add_path(std::string path) {
  if (paths.contains(path)) { return false; }
  paths.emplace(path);
  io::populate_collection(*this, std::filesystem::path(path));
  debug_log(playlist_ids);
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

std::optional<size_t> Collection::find_playlist_index(size_t playlist_id) {
  for (size_t i = 0; i < playlist_ids.size(); i += 1) {
    if (playlist_ids[i] == playlist_id) {
      return i;
    }
  }
  return std::nullopt;
}

void db::serialize(std::ofstream& os) {
  write_bin(os, collections.size());
  for (auto& c : collections) {
    c.serialize(os);
  }
  write_bin(os, playlists.size());
  for (auto& p : playlists) {
    p.serialize(os);
  }
  write_bin(os, tracks.size());
  for (auto& t : tracks) {
    t.serialize(os);
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
