#include "io.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <spng.h>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/mp4/mp4file.h>
#include <taglib/mpeg/id3v2/frames/attachedpictureframe.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/ogg/vorbis/vorbisfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tbytevector.h>
#include <taglib/toolkit/tfilestream.h>
#include <taglib/toolkit/tpropertymap.h>
#include <taglib/toolkit/tstring.h>
#include "common/logger.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"

namespace fs = std::filesystem;

std::optional<fs::path> user_data_path;

bool io::is_cover_file(const fs::path& path) {
  auto ext = path.extension();
  return (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".jpeg" || ext == ".JPEG" ||
          ext == ".bmp" || ext == ".BMP" || ext == ".webp" || ext == ".WEBP");
}

bool io::is_music_file(const fs::path& path) {
  auto ext = path.extension();
  return (ext == ".mp3" || ext == ".MP3" || ext == ".flac" || ext == ".FLAC" || ext == ".ogg" || ext == ".OGG" ||
          ext == ".wav" || ext == ".WAV");
}

void io::open_folder_in_file_manager(const fs::path& path) {
#ifdef _WIN32
  std::string command = "explorer \"" + path.string() + "\"";
#elif __APPLE__
  std::string command = "open \"" + path.string() + "\"";
#else
  std::string command = "xdg-open \"" + path.string() + "\"";
#endif
  std::system(command.c_str());
}

fs::path io::get_user_data_path() {
  if (user_data_path.has_value()) { return user_data_path.value(); }

#if defined(_WIN32)
  const std::string path = std::string(getenv("APPDATA")) + "\\zincbox\\";
#elif defined(__APPLE__)
  const std::string path = std::string(getenv("HOME")) + "/Library/Application Support/zincbox/";
#elif defined(__linux__)
  const char* xdg_data = getenv("XDG_DATA_HOME");
  const std::string path =
    (xdg_data ? std::string(xdg_data) : std::string(getenv("HOME")) + "/.local/share") + "/zincbox/";
#else
#error "unknown platform"
#endif

  if (!fs::exists(fs::path(path))) {
    if (!fs::create_directories(fs::path(path))) {
      out::log_critical("failed to create user data directory");
      exit(1);
    }
  } else if (!fs::is_directory(fs::path(path))) {
    out::log_critical("user data directory exists but is not a directory");
    exit(1);
  }

  user_data_path = fs::path(path);
  return user_data_path.value();
}

fs::path io::get_themes_path() {
  fs::path path = get_user_data_path() / "themes";

  if (!fs::exists(fs::path(path))) {
    if (!fs::create_directories(fs::path(path))) {
      out::log_critical("failed to create themes directory");
      exit(1);
    }
  } else if (!fs::is_directory(fs::path(path))) {
    out::log_critical("themes directory exists but is not a directory");
    exit(1);
  }

  return path;
}

fs::path io::get_db_path() { return get_user_data_path() / "zincbox.db"; }

fs::path io::get_cfg_path() { return get_user_data_path() / "zincbox.json"; }

fs::path io::get_cover_cache_path() {
  fs::path path = get_user_data_path() / "cover_cache";

  if (!fs::exists(fs::path(path))) {
    if (!fs::create_directories(fs::path(path))) {
      out::log_critical("failed to create cover_cache directory");
      exit(1);
    }
  } else if (!fs::is_directory(fs::path(path))) {
    out::log_critical("cover_cache directory exists but is not a directory");
    exit(1);
  }

  return path;
}
