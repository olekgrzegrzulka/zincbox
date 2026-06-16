#pragma once
#include <filesystem>

namespace db {
  struct Playlist;
}

namespace TagLib {
  class FileRef;
}

namespace io {
  namespace fs = std::filesystem;

  bool is_cover_file(const fs::path&);
  bool is_music_file(const fs::path&);

  fs::path get_user_data_path();
  fs::path get_themes_path();
  fs::path get_db_path();
  fs::path get_cfg_path();
  fs::path get_cover_cache_path();
} // namespace io
