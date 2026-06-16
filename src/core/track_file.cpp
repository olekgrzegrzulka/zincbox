#include <taglib/mpeg/id3v2/frames/attachedpictureframe.h> // doens't compile if in order???

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4/mp4file.h>
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
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/musicdb/track.hpp"
#include "lib/libspng/spng/spng.h"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"
#include "track_file.hpp"

namespace fs = std::filesystem;

std::optional<TagLib::ByteVector> get_picture_frame(const TagLib::FileRef&);

static Random rng{};

TrackFile::TrackFile(TrackFile&& other) noexcept
  : track(std::move(other.track)), album_art_64x64(std::move(other.album_art_64x64)),
    album_name(std::move(other.album_name)), album_artist(std::move(other.album_artist)) {}

TrackFile& TrackFile::operator=(TrackFile&& other) noexcept {
  if (this != &other) {
    track = std::move(other.track);
    album_art_64x64 = std::move(other.album_art_64x64);
    album_name = std::move(other.album_name);
    album_artist = std::move(other.album_artist);
  }
  return *this;
}

TrackFile::TrackFile(const fs::path& path, bool fetch_album_art) {
  this->path = path;
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

  if (fetch_album_art) { this->fetch_album_art(&f); }
}

bool TrackFile::fetch_album_art(TagLib::FileRef* ref) {
  if (ref) {
    auto picture_frame = get_picture_frame(*ref);
    if (picture_frame.has_value()) {
      album_art_64x64 =
        resize_album_art_to_64x64(reinterpret_cast<const u8*>(picture_frame->data()), (int)picture_frame->size());
      album_art_path = save_album_art(reinterpret_cast<const u8*>(picture_frame->data()), (int)picture_frame->size());
    }
  } else {
    TagLib::FileStream fstream(path.c_str(), true);
    TagLib::FileRef f(&fstream);
    auto picture_frame = get_picture_frame(f);
    if (picture_frame.has_value()) {
      album_art_64x64 =
        resize_album_art_to_64x64(reinterpret_cast<const u8*>(picture_frame->data()), (int)picture_frame->size());
      album_art_path = save_album_art(reinterpret_cast<const u8*>(picture_frame->data()), (int)picture_frame->size());
    }
  }
  return album_art_path.has_value() && !album_art_64x64.empty();
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

std::optional<fs::path> TrackFile::save_album_art(const u8* data, size_t size) {
  int width, height, channels;
  u8* img = stbi_load_from_memory(data, size, &width, &height, &channels, STBI_rgb_alpha);
  auto res = TrackFile::save_album_art(img, width, height, channels);
  stbi_image_free(img);
  return res;
}

std::optional<fs::path> TrackFile::save_album_art(stbi_uc* img, i32 width, i32 height, i32 /* channels */) {
  if (!img) { return std::nullopt; }
  std::vector<u8> img_resized;
  double scale = std::min(512.0 / width, 512.0 / height);
  i32 w = width * scale;
  i32 h = height * scale;
  img_resized.resize((long)4 * w * h);
  stbir_resize_uint8_linear(img, width, height, 0, img_resized.data(), w, h, 0, STBIR_RGBA);
  auto path = io::get_cover_cache_path();

  size_t filename = rng.next<size_t>(1000000000, 9999999999);
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
    .width = w,
    .height = h,
    .bit_depth = 8,
    .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA,
    .compression_method = 0,
    .filter_method = 0,
    .interlace_method = 0,
  };
  spng_set_ihdr(enc_ctx, &ihdr);
  size_t image_size = (size_t)(w * h) * 4;
  int error = spng_encode_image(enc_ctx, img_resized.data(), image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
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

std::vector<u8> TrackFile::resize_album_art_to_64x64(const u8* data, size_t size) {
  if (size == 0) { return {}; }

  int width, height, channels;
  auto img = stbi_load_from_memory(data, size, &width, &height, &channels, STBI_rgb_alpha);
  auto res = TrackFile::resize_album_art_to_64x64(img, width, height, channels);
  stbi_image_free(img);
  return res;
}

std::vector<u8> TrackFile::resize_album_art_to_64x64(stbi_uc* img, i32 width, i32 height, i32 /* channels */) {
  if (!img) { return {}; }

  std::vector<u8> resized((size_t)(64 * 64) * 4);
  stbir_resize_uint8_linear(img, width, height, 0, resized.data(), 64, 64, 0, STBIR_RGBA);

  return resized;
}
