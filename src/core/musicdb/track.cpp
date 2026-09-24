
#include "track.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/musicdb/types.hpp"

namespace fs = std::filesystem;

db::Track::Track() {}

db::Track::Track(std::ifstream& is) {
  std::string file_path_utf8;
  read_bin(is, originating_album_id);
  read_str(is, file_path_utf8);
  file_path = utf8_to_path(file_path_utf8);
  read_bin(is, file_size);
  read_bin(is, last_modified);
  read_str(is, metadata.title);
  read_str(is, metadata.artist);
  read_str(is, metadata.album);
  size_t album_artist_size = 0;
  if (album_artist_size > 1024) {
    out::critical("Database corrupted: invalid album_artist_size");
    exit(1);
  }
  read_bin<size_t>(is, album_artist_size);
  metadata.album_artist.resize(album_artist_size);
  for (size_t i = 0; i < album_artist_size; i += 1) {
    read_str(is, metadata.album_artist[i]);
  }
  read_str(is, metadata.genre);
  i32 track_num = -1;
  i32 track_tot = -1;
  i32 yr = -1;
  read_bin(is, track_num);
  read_bin(is, track_tot);
  read_bin(is, yr);

  metadata.track_number = (track_num >= 0) ? std::optional<i32>(track_num) : std::nullopt;
  metadata.track_total = (track_tot >= 0) ? std::optional<i32>(track_tot) : std::nullopt;
  metadata.year = (yr >= 0) ? std::optional<int32_t>(yr) : std::nullopt;
  read_bin(is, metadata.duration_ms);
  read_bin(is, metadata.bitrate_kb_s);
}

void db::Track::serialize(std::ostream& os, std::optional<std::span<size_t>> old_playlist_id_to_new_playlist_id) const {
  auto origin = originating_album_id;
  if (old_playlist_id_to_new_playlist_id.has_value() && originating_album_id != db::INVALID_ID) {
    const auto& id_map = *old_playlist_id_to_new_playlist_id;
    if (originating_album_id < id_map.size()) {
      origin = id_map[originating_album_id];
    } else {
      origin = db::INVALID_ID;
    }
  }
  std::string file_path_utf8 = path_to_utf8(file_path);

  write_bin(os, origin);
  write_str(os, file_path_utf8);
  write_bin(os, file_size);
  write_bin(os, last_modified);
  write_str(os, metadata.title);
  write_str(os, metadata.artist);
  write_str(os, metadata.album);
  write_bin<size_t>(os, metadata.album_artist.size());
  for (auto& aa : metadata.album_artist) {
    write_str(os, aa);
  }
  write_str(os, metadata.genre);
  write_bin(os, metadata.track_number.value_or(-1));
  write_bin(os, metadata.track_total.value_or(-1));
  write_bin(os, metadata.year.value_or(-1));
  write_bin(os, metadata.duration_ms);
  write_bin(os, metadata.bitrate_kb_s);
}

std::string db::Track::to_string() const {
  return std::to_string(metadata.track_number.value_or(0)) + ". " + metadata.artist + " - " + metadata.title +
         (get_flag(TOMBSTONE) ? " (tombstone)" : "");
}

std::string db::Track::pretty_title() const {
  if (!metadata.title.empty()) {
    return metadata.title;
  } else {
    return path_to_utf8(file_name_with_extension());
  }
}

std::string db::Track::pretty_name() const {
  if (!metadata.title.empty()) {
    if (!metadata.artist.empty()) {
      return metadata.artist + " - " + metadata.title;
    } else {
      return metadata.title;
    }
  } else {
    return path_to_utf8(file_name_with_extension());
  }
}
std::string db::Track::pretty_length() const {
  i32 length_s = metadata.duration_ms / 1000;
  i32 length_m = metadata.duration_ms / 1000 / 60;
  length_s %= 60;
  std::stringstream ss;
  ss << std::right << std::setfill('0') << std::setw(0) << length_m << ":" << std::setw(2) << length_s;
  return ss.str();
}

std::string db::Track::pretty_album_artist() const {
  std::string ret;
  for (auto& a : metadata.album_artist) {
    if (a.empty()) { continue; }
    ret += a + ", ";
  }
  if (!ret.empty()) { ret.pop_back(); }
  if (!ret.empty()) { ret.pop_back(); }
  return ret;
}
