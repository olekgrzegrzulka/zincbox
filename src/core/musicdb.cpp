#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "../debug.hpp"
#include "../types.hpp"
#include "../utf.hpp"
#include "musicdb.hpp"

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

std::optional<size_t> Collection::find_playlist_index(size_t playlist_id) {
  for (size_t i = 0; i < playlist_ids.size(); i += 1) {
    if (playlist_ids[i] == playlist_id) {
      return i;
    }
  }
  return std::nullopt;
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
