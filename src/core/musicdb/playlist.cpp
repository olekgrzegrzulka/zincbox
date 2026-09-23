#include "playlist.hpp"
#include <fstream>
#include <optional>
#include <vector>
#include "common/logger.hpp"
#include "common/serialize.hpp"
#include "core/musicdb/musicdb.hpp"
#include "lib/stb_image/stb_image.h"

db::Playlist::Playlist(std::ifstream& is) {
  read_str(is, name);
  size_t author_size = 0;
  read_bin(is, author_size);
  if (author_size > 1024) {
    out::critical("Database corrupted: invalid author_size");
    exit(1);
  }
  author.resize(author_size);
  for (size_t i = 0; i < author_size; i += 1) {
    read_str(is, author[i]);
  }

  read_blob(is, art_64x64);
  read_bin(is, type);
  size_t track_ids_size = 0;
  read_bin(is, track_ids_size);
  if (track_ids_size > 16384) {
    out::critical("Database corrupted: invalid track_ids_size");
    exit(1);
  }
  track_ids.resize(track_ids_size);
  for (size_t i = 0; i < track_ids_size; i += 1) {
    size_t value;
    read_bin(is, value);
    track_ids[i] = value;
  }
  read_str(is, art_file_path);
}

bool db::Playlist::add_track(size_t track_id) {
  if (has_track_id(track_id)) { return false; }
  track_ids.emplace_back(track_id);
  return true;
}

void db::Playlist::insert_tracks(size_t at, std::span<const size_t> new_track_ids) {
  if (new_track_ids.empty()) { return; }
  size_t insert_pos = std::min(at, track_ids.size());
  std::vector<size_t> unique_tracks;
  unique_tracks.reserve(new_track_ids.size());
  for (size_t id : new_track_ids) {
    if (!has_track_id(id) && std::find(unique_tracks.begin(), unique_tracks.end(), id) == unique_tracks.end()) {
      unique_tracks.emplace_back(id);
    }
  }
  if (!unique_tracks.empty()) {
    track_ids.insert(track_ids.begin() + insert_pos, unique_tracks.begin(), unique_tracks.end());
  }
}

bool db::Playlist::remove_track_by_id(size_t track_id) {
  auto index = find_track_index(track_id);
  if (!index.has_value()) { return false; }
  track_ids.erase(track_ids.begin() + index.value());
  return true;
}

bool db::Playlist::remove_track_by_index(size_t index) {
  if (index >= track_ids.size()) { return false; }
  track_ids.erase(track_ids.begin() + index);
  return true;
}

void db::Playlist::remove_tracks_by_indices(std::span<const size_t> indices) {
  if (indices.empty() || track_ids.empty()) { return; }
  std::vector<size_t> sorted_indices(indices.begin(), indices.end());
  std::sort(sorted_indices.begin(), sorted_indices.end(), std::greater<size_t>());
  sorted_indices.erase(std::unique(sorted_indices.begin(), sorted_indices.end()), sorted_indices.end());
  for (size_t index : sorted_indices) {
    if (index < track_ids.size()) { track_ids.erase(track_ids.begin() + index); }
  }
}

void db::Playlist::sort_by_track_number() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    if (lhs.metadata.track_number != rhs.metadata.track_number) {
      return lhs.metadata.track_number < rhs.metadata.track_number;
    } else if (!lhs.metadata.artist.empty() && !rhs.metadata.artist.empty() && !lhs.metadata.title.empty() &&
               !rhs.metadata.title.empty()) {
      return std::tie(lhs.metadata.artist, lhs.metadata.title) < std::tie(rhs.metadata.artist, rhs.metadata.title);
    } else {
      return lhs.pretty_name() < rhs.pretty_name();
    }
  });
}

void db::Playlist::sort_by_artist_asc() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) -> bool {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    return std::tie(lhs.metadata.artist, lhs.metadata.title) < std::tie(rhs.metadata.artist, rhs.metadata.title);
  });
}

void db::Playlist::sort_by_artist_desc() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) -> bool {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    return std::tie(lhs.metadata.artist, lhs.metadata.title) > std::tie(rhs.metadata.artist, rhs.metadata.title);
  });
}

void db::Playlist::sort_by_name_asc() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) -> bool {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    return std::tie(lhs.metadata.title, lhs.metadata.artist) < std::tie(rhs.metadata.title, rhs.metadata.artist);
  });
}

void db::Playlist::sort_by_name_desc() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) -> bool {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    return std::tie(lhs.metadata.title, lhs.metadata.artist) > std::tie(rhs.metadata.title, rhs.metadata.artist);
  });
}

std::string db::Playlist::author_pretty() const {
  std::string ret;
  for (auto& a : author) {
    if (a.empty()) { continue; }
    ret += a + ", ";
  }
  if (!ret.empty()) { ret.pop_back(); }
  if (!ret.empty()) { ret.pop_back(); }
  return ret;
}

std::optional<size_t> db::Playlist::next_track_id(size_t track_id) const {
  auto index = find_track_index(track_id);
  if (index.has_value() && index.value() + 1 < track_ids.size()) {
    return track_ids[index.value() + 1];
  } else {
    return std::nullopt;
  }
}

std::optional<size_t> db::Playlist::prev_track_id(size_t track_id) const {
  auto index = find_track_index(track_id);
  if (index.has_value() && index.value() > 0) {
    return track_ids[index.value() - 1];
  } else {
    return std::nullopt;
  }
}

bool db::Playlist::has_track_id(size_t track_id) const {
  auto index = find_track_index(track_id);
  if (index.has_value()) {
    return true;
  } else {
    return false;
  }
}

std::optional<size_t> db::Playlist::find_track_index(size_t track_id) const {
  for (size_t i = 0; i < track_ids.size(); i += 1) {
    if (track_ids[i] == track_id) { return i; }
  }
  return std::nullopt;
}

void db::Playlist::serialize(std::ostream& os) const {
  write_str(os, name);
  write_bin(os, author.size());
  for (auto& a : author) {
    write_str(os, a);
  }
  write_blob(os, art_64x64);
  write_bin(os, type);
  write_bin(os, track_ids.size());
  for (size_t track_id : track_ids) {
    write_bin(os, track_id);
  }
  write_str(os, art_file_path);
}

void db::Playlist::serialize(std::ostream& os, const std::vector<size_t>& old_track_id_to_new_track_id) const {
  write_str(os, name);
  write_bin(os, author.size());
  for (auto& a : author) {
    write_str(os, a);
  }
  write_blob(os, art_64x64);
  write_bin(os, type);
  write_bin(os, track_ids.size());
  for (size_t track_id : track_ids) {
    write_bin(os, old_track_id_to_new_track_id[track_id]);
  }
  write_str(os, art_file_path);
}
