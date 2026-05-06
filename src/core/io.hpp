
#include <filesystem>
#include <optional>
#include <string_view>
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
#include "core/utf.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"
#include "mpeg/id3v2/id3v2tag.h"
#include "musicdb.hpp"

namespace fs = std::filesystem;

namespace io {
  inline std::vector<u8> resize_album_art_to_64x64(const std::vector<u8>& raw_data) {
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

  inline std::vector<u8> fetch_album_art(const TagLib::FileRef& ref) {
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

  inline bool is_cover_file(fs::path path) {
    auto ext = path.extension();
    return (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".jpeg" || ext == ".JPEG" || ext == ".bmp" || ext == ".BMP" || ext == ".webp" || ext == ".WEBP");
  }

  inline bool is_music_file(fs::path path) {
    auto ext = path.extension();
    return (ext == ".mp3" || ext == ".MP3" || ext == ".flac" || ext == ".FLAC" || ext == ".ogg" || ext == ".OGG" || ext == ".wav" || ext == ".WAV");
  }

  inline bool add_music_file(db::Playlist& playlist, fs::path path) {
    TagLib::FileStream fstream(path.c_str(), true);
    TagLib::FileRef f(&fstream);

    if (f.isNull()) { return false; }

    TagLib::Tag* tag = f.tag();
    TagLib::AudioProperties* audio_props = f.audioProperties();
    TagLib::PropertyMap properties = f.file()->properties();

    std::string album_artist;
    if (properties.contains("ALBUMARTIST")) {
      TagLib::StringList artistList = properties["ALBUMARTIST"];
      for (const TagLib::String& artist : artistList) {
        album_artist.append(artist.to8Bit(true));
        album_artist.append(", ");
      }
      if (album_artist.size() >= 2) {
        album_artist.pop_back();
        album_artist.pop_back();
      }
    }

    size_t track_id = db::add_track((i32)tag->track(),
                                    utf8_to_utf32(tag->title().to8Bit(true)),
                                    utf8_to_utf32(tag->artist().to8Bit(true)),
                                    utf8_to_utf32(album_artist),
                                    utf8_to_utf32(tag->genre().to8Bit(true)),
                                    (i32)tag->year(),
                                    audio_props->bitrate(),
                                    audio_props->lengthInSeconds(),
                                    utf8_to_utf32(path.string()));

    if (playlist.image.empty()) {
      playlist.image = fetch_album_art(f);
    }
    if (playlist.author.empty()) {
      playlist.author = utf8_to_utf32(album_artist);
    }
    if (playlist.name.empty()) {
      playlist.name = utf8_to_utf32(tag->album().to8Bit(true));
    }
    playlist.add_track(track_id);
    return true;
  }

  inline bool add_cover_file(db::Playlist& playlist, fs::path path) {
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
} // namespace io
