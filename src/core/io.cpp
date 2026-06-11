#include "io.hpp"
#include <cstdio>
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
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/random.hpp"
#include "common/utf.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/track.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"
#include "mpeg/id3v2/id3v2tag.h"

namespace fs = std::filesystem;

std::optional<fs::path> user_data_path;

std::vector<u8> io::resize_album_art_to_64x64(const std::vector<u8>& raw_data) {
  if (raw_data.empty()) { return {}; }

  int width, height, channels;
  u8* img = stbi_load_from_memory(raw_data.data(), (int)raw_data.size(), &width, &height, &channels, STBI_rgb_alpha);
  if (!img) {
    stbi_image_free(img);
    return {};
  }

  std::vector<u8> resized(64 * 64 * 4);
  stbir_resize_uint8_linear(img, width, height, 0, resized.data(), 64, 64, 0, STBIR_RGBA);

  stbi_image_free(img);
  return resized;
}

std::optional<TagLib::ByteVector> get_picture_frame(const TagLib::FileRef& ref) {
  if (auto* mpeg = dynamic_cast<TagLib::MPEG::File*>(ref.file())) {
    if (mpeg->ID3v2Tag()) {
      auto frames = mpeg->ID3v2Tag()->frameList("APIC");
      if (!frames.isEmpty()) {
        auto* frame = reinterpret_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
        auto data = frame->picture();
        return data;
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
          return data;
        }
      }
    }
  }

  if (auto* flac = dynamic_cast<TagLib::FLAC::File*>(ref.file())) {
    const auto& pics = flac->pictureList();
    if (!pics.isEmpty()) {
      auto data = pics.front()->data();
      return data;
    }
  }

  if (auto* mp4 = dynamic_cast<TagLib::MP4::File*>(ref.file())) {
    if (mp4->tag()) {
      auto items = mp4->tag()->itemMap();
      if (items.contains("covr")) {
        auto coverList = items["covr"].toCoverArtList();
        if (!coverList.isEmpty()) {
          auto data = coverList.front().data();
          return data;
        }
      }
    }
  }
  return std::nullopt;
}

std::vector<u8> io::fetch_album_art(const TagLib::FileRef& ref) {
  auto picture_frame = get_picture_frame(ref);
  if (picture_frame.has_value()) {
    auto art = std::vector<u8>(picture_frame->data(), picture_frame->data() + picture_frame->size());
    auto resized = resize_album_art_to_64x64(art);
    return resized;
  }
  return {};
}

std::optional<fs::path> io::save_album_art(const TagLib::FileRef& ref) {
  auto picture_frame = get_picture_frame(ref);
  if (!picture_frame) { return std::nullopt; }

  int width, height, channels;
  auto art = std::vector<u8>(picture_frame->data(), picture_frame->data() + picture_frame->size());
  u8* img = stbi_load_from_memory(art.data(), (int)art.size(), &width, &height, &channels, STBI_rgb_alpha);
  if (!img) {
    stbi_image_free(img);
    return std::nullopt;
  }

  auto path = io::get_cover_cache_path();

  size_t filename = StaticRandom::get().next<size_t>(1000000000, 9999999999);
  std::filesystem::path file_path = std::filesystem::path(path) / (std::to_string(filename) + ".png");

  FILE* out_file = fopen(file_path.string().c_str(), "wb");
  if (!out_file) { return std::nullopt; }

  spng_ctx* enc_ctx = spng_ctx_new(SPNG_CTX_ENCODER);
  if (!enc_ctx) {
    fclose(out_file);
    return std::nullopt;
  }

  spng_set_png_file(enc_ctx, out_file);
  struct spng_ihdr ihdr = {
    .width = width,
    .height = height,
    .bit_depth = 8,
    .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA,
    .compression_method = 0,
    .filter_method = 0,
    .interlace_method = 0,
  };
  spng_set_ihdr(enc_ctx, &ihdr);
  size_t image_size = width * height * 4;
  int error = spng_encode_image(enc_ctx, img, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
  if (error != 0) {
    out::debug_error("spng_encode_image error {} {}", error, spng_strerror(error));
    stbi_image_free(img);
    spng_ctx_free(enc_ctx);
    fclose(out_file);
    return std::nullopt;
  }

  stbi_image_free(img);
  spng_ctx_free(enc_ctx);
  fclose(out_file);

  return file_path;
}

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

io::TrackFile::TrackFile(TrackFile&& other) noexcept
  : track(std::move(other.track)), album_art(std::move(other.album_art)), album_name(std::move(other.album_name)),
    album_artist(std::move(other.album_artist)) {}

io::TrackFile& io::TrackFile::operator=(TrackFile&& other) noexcept {
  if (this != &other) {
    track = std::move(other.track);
    album_art = std::move(other.album_art);
    album_name = std::move(other.album_name);
    album_artist = std::move(other.album_artist);
  }
  return *this;
}

io::TrackFile::TrackFile(const fs::path& path) {
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
    (i32)tag->track(),      utf8_to_utf32(tag->title().to8Bit(true)), utf8_to_utf32(tag->artist().to8Bit(true)),
    album_artist,           utf8_to_utf32(tag->genre().to8Bit(true)), (i32)tag->year(),
    audio_props->bitrate(), audio_props->lengthInSeconds(),           utf8_to_utf32(path.string()),
  };
}

std::vector<u8> io::TrackFile::get_album_art() {
  TagLib::FileStream fstream(utf32_to_utf8(track->path).c_str(), true);
  TagLib::FileRef f(&fstream);
  if (album_art.empty()) { album_art = fetch_album_art(f); }
  return album_art;
}

std::optional<fs::path> io::TrackFile::save_album_art() {
  TagLib::FileStream fstream(utf32_to_utf8(track->path).c_str(), true);
  TagLib::FileRef f(&fstream);
  return io::save_album_art(f);
}

std::optional<fs::path> save_album_art(stbi_uc* data, i32 width, i32 height) {
  auto path = io::get_cover_cache_path();

  size_t filename = StaticRandom::get().next<size_t>(1000000000, 9999999999);
  std::filesystem::path file_path = std::filesystem::path(path) / (std::to_string(filename) + ".png");
  FILE* out_file = fopen(file_path.string().c_str(), "wb");
  if (!out_file) { return std::nullopt; }

  spng_ctx* enc_ctx = spng_ctx_new(SPNG_CTX_ENCODER);
  if (!enc_ctx) {
    fclose(out_file);
    return std::nullopt;
  }

  spng_set_png_file(enc_ctx, out_file);
  struct spng_ihdr ihdr = {
    .width = width,
    .height = height,
    .bit_depth = 8,
    .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA,
    .compression_method = 0,
    .filter_method = 0,
    .interlace_method = 0,
  };
  spng_set_ihdr(enc_ctx, &ihdr);
  size_t image_size = width * height * 4;
  int error = spng_encode_image(enc_ctx, data, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
  if (error != 0) {
    out::debug_error("spng_encode_image error {} {}", error, spng_strerror(error));
    spng_ctx_free(enc_ctx);
    fclose(out_file);
    return std::nullopt;
  }

  spng_ctx_free(enc_ctx);
  fclose(out_file);

  return file_path;
}

bool io::add_cover_file(db::Playlist& playlist, const fs::path& path) {
  if (!playlist.image.empty()) { return false; }
  i32 width, height, channels;
  stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!data) { return false; }
  auto cover_path = ::save_album_art(data, width, height);
  std::vector<u8> data_64x64{};
  data_64x64.resize(64 * 64 * 4);
  stbir_resize_uint8_linear(data, width, height, 0, data_64x64.data(), 64, 64, 0, STBIR_RGBA);
  stbi_image_free(data);

  playlist.image = data_64x64;
  playlist.cover_file_path = utf8_to_utf32(cover_path->string());
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

fs::path io::get_cfg_path() { return get_user_data_path() / "zincbox.cfg"; }

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
