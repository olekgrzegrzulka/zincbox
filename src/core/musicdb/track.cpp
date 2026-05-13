
#include "track.hpp"
#include <fstream>
#include <string>
#include "common/serialize.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"

db::Track::Track(i32 track_number,
                 std::u32string title,
                 std::u32string artist,
                 std::u32string album_artist,
                 std::u32string genre,
                 i32 year,
                 i32 bitrate,
                 i32 length_seconds,
                 std::u32string path) {
  this->track_number = track_number;
  this->title = title;
  this->artist = artist;
  this->album_artist = album_artist;
  this->genre = genre;
  this->year = year;
  this->bitrate = bitrate;
  this->length_seconds = length_seconds;
  this->path = path;
}

db::Track::Track(std::ifstream& is) {
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

void db::Track::serialize(std::ostream& os) const {
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
  return std::to_string(track_number) + ". " + utf32_to_utf8(artist) + " - " + utf32_to_utf8(title) + (tombstone ? " (tombstone)" : "");
}
