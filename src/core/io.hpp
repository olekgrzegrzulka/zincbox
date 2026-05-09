#pragma once
#include <filesystem>
#include <vector>
#include "common/types.hpp"

namespace db {
  struct Playlist;
}

namespace TagLib {
  class FileRef;
}

namespace io {
  namespace fs = std::filesystem;

  std::vector<u8> resize_album_art_to_64x64(const std::vector<u8>& raw_data);
  std::vector<u8> fetch_album_art(const TagLib::FileRef&);
  bool is_cover_file(fs::path path);
  bool is_music_file(fs::path path);
  bool add_music_file(db::Playlist& playlist, fs::path path);
  bool add_cover_file(db::Playlist& playlist, fs::path path);

  fs::path get_user_data_path();
  fs::path get_themes_path();

} // namespace io
