
#include "track.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/musicdb/types.hpp"

db::Track::Track(i32 track_number_, std::u32string title_, std::u32string artist_, std::u32string album_artist_,
                 std::u32string genre_, i32 year_, i32 bitrate_, i32 length_seconds_, std::u32string path_) {
  track_number = track_number_;
  title = std::move(title_);
  artist = std::move(artist_);
  album_artist = std::move(album_artist_);
  genre = std::move(genre_);
  year = year_;
  bitrate = bitrate_;
  length_seconds = length_seconds_;
  path = std::move(path_);
}

db::Track::Track(std::ifstream& is) {
  read_bin(is, originating_album_id);
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

void db::Track::serialize(std::ostream& os, std::optional<std::span<size_t>> old_playlist_id_to_new_playlist_id) const {
  auto origin = db::INVALID_ID;
  if (old_playlist_id_to_new_playlist_id.has_value() && originating_album_id != db::INVALID_ID) {
    origin = old_playlist_id_to_new_playlist_id.value()[originating_album_id];
  } else {
    origin = INVALID_ID;
  }
  write_bin(os, origin);
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

std::string db::Track::to_string() const {
  return std::to_string(track_number) + ". " + utf32_to_utf8(artist) + " - " + utf32_to_utf8(title) +
         (is_tombstone() ? " (tombstone)" : "");
}

jt::Json db::Track::to_json() const {
  jt::Json json;
  json["trackNumber"] = track_number;
  json["title"] = utf32_to_utf8(title);
  json["artist"] = utf32_to_utf8(artist);
  json["albumArtist"] = utf32_to_utf8(album_artist);
  json["genre"] = utf32_to_utf8(genre);
  json["year"] = year;
  json["bitrate"] = bitrate;
  json["lengthSeconds"] = length_seconds;
  json["path"] = utf32_to_utf8(path);
  return json;
}

std::u32string db::Track::pretty_name() const {
  if (!title.empty()) {
    if (!artist.empty()) {
      return artist + U" - " + title;
    } else {
      return title;
    }
  } else {
    return utf8_to_utf32(std::filesystem::path(path).filename().string());
  }
}
std::u32string db::Track::pretty_length() const {
  i32 length_s = length_seconds;
  i32 length_m = length_seconds / 60;
  length_s %= 60;
  std::stringstream ss;
  ss << std::right << std::setfill('0') << std::setw(0) << length_m << ":" << std::setw(2) << length_s;
  return utf8_to_utf32(ss.str());
}
