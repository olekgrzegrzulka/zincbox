#include <fstream>
#include <string>
#include <vector>
#include "debug.hpp"
#include "musicdb.hpp"
#include "musicdb_serialize.hpp"

template <typename T>
void write_bin(std::ostream& os, const T& value) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void write_str(std::ostream& os, const std::basic_string<T>& s) {
  uint64_t count = s.size();
  write_bin(os, count);
  os.write(reinterpret_cast<const char*>(s.data()), count * sizeof(T));
}

void write_blob(std::ostream& os, const std::vector<u8>& v) {
  uint64_t size = v.size();
  write_bin(os, size);
  os.write(reinterpret_cast<const char*>(v.data()), size);
}

template <typename T>
void read_bin(std::istream& is, T& value) {
  is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template <typename T>
void read_str(std::istream& is, std::basic_string<T>& s) {
  uint64_t count;
  read_bin(is, count);
  s.resize(count);
  if (count > 0) {
    is.read(reinterpret_cast<char*>(&s[0]), count * sizeof(T));
  }
}

void read_blob(std::istream& is, std::vector<uint8_t>& v) {
  uint64_t size;
  read_bin(is, size);
  v.resize(size);
  is.read(reinterpret_cast<char*>(v.data()), size);
}

void musicdb::save_to_file(const std::string& path) {
  std::ofstream os(path, std::ios::binary);

  uint64_t album_count = musicdb::get_albums().size();
  write_bin(os, album_count);

  for (const auto& album : musicdb::get_albums()) {
    write_str(os, album.title);
    write_blob(os, album.cover_art);

    uint64_t track_count = album.track_ids.size();
    write_bin(os, track_count);

    for (track_id_t track_id : album.track_ids) {
      auto& t = get_tracks()[track_id];

      write_bin(os, t.track_id);
      write_bin(os, t.album_id);
      write_bin(os, t.track_number);
      write_str(os, t.title);
      write_str(os, t.artist);
      write_str(os, t.genre);
      write_bin(os, t.year);
      write_bin(os, t.bitrate);
      write_bin(os, t.length_seconds);
      write_str(os, t.path);
    }
  }
}

void musicdb::load_from_file(const std::string& path) {
  std::ifstream is(path, std::ios::binary);
  if (!is) return;

  musicdb::clear_albums();

  uint64_t album_count;
  read_bin(is, album_count);

  for (uint64_t i = 0; i < album_count; ++i) {
    Album album;
    read_str(is, album.title);
    read_blob(is, album.cover_art);

    uint64_t track_count;
    read_bin(is, track_count);

    for (uint64_t j = 0; j < track_count; ++j) {
      Track t;
      read_bin(is, t.track_id);
      read_bin(is, t.album_id);
      read_bin(is, t.track_number);
      read_str(is, t.title);
      read_str(is, t.artist);
      read_str(is, t.genre);
      read_bin(is, t.year);
      read_bin(is, t.bitrate);
      read_bin(is, t.length_seconds);
      read_str(is, t.path);
      add_track(std::move(t), t.album_id);
    }
    musicdb::add_album(std::move(album));
  }
}
