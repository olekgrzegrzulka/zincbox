#include "io.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/mp4/mp4file.h>
#include <taglib/mpeg/id3v2/frames/attachedpictureframe.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/ogg/vorbis/vorbisfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tfilestream.h>
#include <taglib/toolkit/tpropertymap.h>
#include <taglib/toolkit/tstring.h>
#include "common/debug.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/track.hpp"
#include "core/utf.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"
#include "mpeg/id3v2/id3v2tag.h"

namespace fs = std::filesystem;

std::optional<fs::path> user_data_path;

std::vector<u8> io::resize_album_art_to_64x64(const std::vector<u8>& raw_data) {
  if (raw_data.empty()) return {};

  int width, height, channels;
  u8* img = stbi_load_from_memory(raw_data.data(), (int)raw_data.size(), &width, &height, &channels, STBI_rgb_alpha);
  if (!img) return {};

  std::vector<u8> resized(64 * 64 * 4);
  stbir_resize_uint8_linear(img, width, height, 0,
                            resized.data(), 64, 64, 0,
                            STBIR_RGBA);

  stbi_image_free(img);
  return resized;
}

std::vector<u8> io::fetch_album_art(const TagLib::FileRef& ref) {
  if (auto* mpeg = dynamic_cast<TagLib::MPEG::File*>(ref.file())) {
    if (mpeg->ID3v2Tag()) {
      auto frames = mpeg->ID3v2Tag()->frameList("APIC");
      if (!frames.isEmpty()) {
        auto* frame = reinterpret_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
        auto data = frame->picture();

        auto art = std::vector<u8>(data.data(), data.data() + data.size());
        auto resized = resize_album_art_to_64x64(art);
        return resized;
      }
    }
  }

  if (auto* ogg = dynamic_cast<TagLib::Vorbis::File*>(ref.file())) {
    if (ogg->isValid() && ogg->tag()) {
      TagLib::Ogg::XiphComment* tag = ogg->tag();

      const auto& fieldListMap = tag->fieldListMap();
      if (fieldListMap.contains("METADATA_BLOCK_PICTURE")) {

        auto pictures = tag->pictureList();

        if (!pictures.isEmpty()) {
          TagLib::FLAC::Picture* pic = pictures[0];
          auto data = pic->data();

          auto art = std::vector<u8>(data.data(), data.data() + data.size());
          auto resized = resize_album_art_to_64x64(art);
          return resized;
        }
      }
    }
  }

  if (auto* flac = dynamic_cast<TagLib::FLAC::File*>(ref.file())) {
    const auto& pics = flac->pictureList();
    if (!pics.isEmpty()) {
      auto data = pics.front()->data();
      auto art = std::vector<u8>(data.data(), data.data() + data.size());
      auto resized = resize_album_art_to_64x64(art);
      return resized;
    }
  }

  if (auto* mp4 = dynamic_cast<TagLib::MP4::File*>(ref.file())) {
    if (mp4->tag()) {
      auto items = mp4->tag()->itemMap();
      if (items.contains("covr")) {
        auto coverList = items["covr"].toCoverArtList();
        if (!coverList.isEmpty()) {
          auto data = coverList.front().data();
          auto art = std::vector<u8>(data.data(), data.data() + data.size());
          auto resized = resize_album_art_to_64x64(art);
          return resized;
        }
      }
    }
  }
  return {};
}

bool io::is_cover_file(fs::path path) {
  auto ext = path.extension();
  return (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".jpeg" || ext == ".JPEG" || ext == ".bmp" || ext == ".BMP" || ext == ".webp" || ext == ".WEBP");
}

bool io::is_music_file(fs::path path) {
  auto ext = path.extension();
  return (ext == ".mp3" || ext == ".MP3" || ext == ".flac" || ext == ".FLAC" || ext == ".ogg" || ext == ".OGG" || ext == ".wav" || ext == ".WAV");
}

io::TrackFile::TrackFile(TrackFile&& other)
  : track(std::move(other.track)),
    album_art(std::move(other.album_art)),
    album_name(std::move(other.album_name)),
    album_artist(std::move(other.album_artist)) {}

io::TrackFile& io::TrackFile::operator=(TrackFile&& other) {
  if (this != &other) {
    track = std::move(other.track);
    album_art = std::move(other.album_art);
    album_name = std::move(other.album_name);
    album_artist = std::move(other.album_artist);
  }
  return *this;
}

io::TrackFile::TrackFile(fs::path path) {
  ScopeTimer timer("TrackFile " + std::string(path));

  TagLib::FileStream fstream(path.c_str(), true);
  TagLib::FileRef f(&fstream);

  if (f.isNull()) { return; }

  TagLib::Tag* tag = f.tag();
  TagLib::AudioProperties* audio_props = f.audioProperties();
  TagLib::PropertyMap properties = f.file()->properties();

  if (properties.contains("ALBUMARTIST")) {
    TagLib::StringList artistList = properties["ALBUMARTIST"];
    for (const TagLib::String& artist : artistList) {
      album_artist.append(utf8_to_utf32(artist.to8Bit(true)));
      album_artist.append(U", ");
    }
    if (album_artist.size() >= 2) {
      album_artist.pop_back();
      album_artist.pop_back();
    }
  }

  album_name = utf8_to_utf32(tag->album().to8Bit(true));

  track = db::Track{
    (i32)tag->track(),
    utf8_to_utf32(tag->title().to8Bit(true)),
    utf8_to_utf32(tag->artist().to8Bit(true)),
    album_artist,
    utf8_to_utf32(tag->genre().to8Bit(true)),
    (i32)tag->year(),
    audio_props->bitrate(),
    audio_props->lengthInSeconds(),
    utf8_to_utf32(path.string()),
  };
}

std::vector<u8> io::TrackFile::get_album_art() {
  TagLib::FileStream fstream(utf32_to_utf8(track->path).c_str(), true);
  TagLib::FileRef f(&fstream);
  if (album_art.empty()) {
    album_art = fetch_album_art(f);
  }
  return album_art;
}

bool io::add_cover_file(db::Playlist& playlist, fs::path path) {
  if (!playlist.image.empty()) {
    return false;
  }
  i32 width, height, channels;
  stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!data) { return false; }
  std::vector<u8> data_64x64{};
  data_64x64.resize(64 * 64 * 4);
  stbir_resize_uint8_linear(data, width, height, 0,
                            data_64x64.data(), 64, 64, 0,
                            STBIR_RGBA);
  stbi_image_free(data);

  playlist.image = data_64x64;
  return true;
}

fs::path io::get_user_data_path() {
  if (user_data_path.has_value()) { return user_data_path.value(); }

#if defined(_WIN32)
  const std::string path = std::string(getenv("APPDATA")) + "\\zincbox\\";
#elif defined(__APPLE__)
  const std::string path = std::string(getenv("HOME")) + "/Library/Application Support/zincbox/";
#elif defined(__linux__)
  const char* xdg_data = getenv("XDG_DATA_HOME");
  const std::string path = (xdg_data ? std::string(xdg_data) : std::string(getenv("HOME")) + "/.local/share") + "/zincbox/";
#else
#error "unknown platform"
#endif

  if (!fs::exists(fs::path(path))) {
    if (!fs::create_directories(fs::path(path))) {
      debug_error("failed to create user data directory");
    }
  } else if (!fs::is_directory(fs::path(path))) {
    debug_error("user data directory exists but is not a directory");
  }

  user_data_path = fs::path(path);
  return user_data_path.value();
}

fs::path io::get_themes_path() {
  const fs::path path = get_user_data_path() / "themes";

  if (!fs::exists(fs::path(path))) {
    if (!fs::create_directories(fs::path(path))) {
      debug_error("failed to create themes directory");
    }
  } else if (!fs::is_directory(fs::path(path))) {
    debug_error("themes directory exists but is not a directory");
  }

  return path;
}
