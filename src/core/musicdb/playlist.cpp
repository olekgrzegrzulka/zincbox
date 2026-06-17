#include "playlist.hpp"
#include <fstream>
#include <optional>
#include <vector>
#include "common/logger.hpp"
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/track_file.hpp"
#include "lib/stb_image/stb_image.h"

db::Playlist::Playlist(std::ifstream& is) {
  read_str(is, name);
  read_str(is, author);

  size_t image_size = 0;
  read_bin(is, image_size);
  art_64x64.resize(image_size);
  for (size_t i = 0; i < image_size; i += 1) {
    u8 value;
    read_bin(is, value);
    art_64x64[i] = value;
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
  read_str(is, art_file_path);
}

bool db::Playlist::add_track(size_t track_id) {
  if (has_track_id(track_id)) { return false; }
  track_ids.emplace_back(track_id);
  return true;
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

void db::Playlist::sort_by_track_number() {
  std::sort(track_ids.begin(), track_ids.end(), [](size_t lhs_id, size_t rhs_id) {
    auto& lhs = db::track_by_id(lhs_id)->get();
    auto& rhs = db::track_by_id(rhs_id)->get();
    return std::tie(lhs.track_number) < std::tie(rhs.track_number);
  });
}

bool db::Playlist::fetch_cover_art(const fs::path& path) {
  i32 width, height, channels;
  stbi_uc* img = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (img == NULL) {
    out::debug_error("Playlist::fetch_cover_art({}): {}", path.string(), stbi_failure_reason());
    return false;
  }
  art_64x64 = TrackFile::resize_album_art_to_64x64(img, width, height, channels);
  auto path_art = TrackFile::save_album_art(img, width, height, channels);
  stbi_image_free(img);
  if (path_art.has_value()) {
    art_file_path = utf8_to_utf32(path_art.value().string());
  } else {
    path_art->clear();
  }
  return !art_64x64.empty() && !art_file_path.empty();
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
  write_str(os, author);
  write_bin(os, art_64x64.size());
  for (auto a : art_64x64) {
    write_bin(os, a);
  }
  write_bin(os, type);
  write_bin(os, track_ids.size());
  for (size_t track_id : track_ids) {
    write_bin(os, track_id);
  }
  write_str(os, art_file_path);
}

void db::Playlist::serialize(std::ostream& os, const std::vector<size_t>& old_track_id_to_new_track_id) const {
  write_str(os, name);
  write_str(os, author);
  write_blob(os, art_64x64);
  write_bin(os, type);
  write_bin(os, track_ids.size());
  for (size_t track_id : track_ids) {
    write_bin(os, old_track_id_to_new_track_id[track_id]);
  }
  write_str(os, art_file_path);
}
