
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
#include "../../lib/stb_image.h"
#include "../../lib/stb_image_resize2.h"
#include "../../lib/stb_image_write.h"
#include "../utf.hpp"
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

  inline void populate_collection(std::optional<std::reference_wrapper<db::Collection>> c, fs::path path) {
    // scan for cover art
    std::vector<u8> cover_art_from_image_in_directory{};
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_regular_file()) {
        auto ext = entry.path().extension();
        bool is_img = (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".jpeg" || ext == ".JPEG" || ext == ".bmp" || ext == ".BMP" || ext == ".webp" || ext == ".WEBP");
        if (is_img) {
          i32 width, height, channels;
          stbi_uc* data = stbi_load(entry.path().c_str(), &width, &height, &channels, STBI_rgb_alpha);
          if (!data) { continue; }
          cover_art_from_image_in_directory.resize(64 * 64 * 4);
          stbir_resize_uint8_linear(data, width, height, 0,
                                    cover_art_from_image_in_directory.data(), 64, 64, 0,
                                    STBIR_RGBA);
          stbi_image_free(data);
          break;
        }
      }
    }

    // scan for music files and other directories
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_directory()) {
        populate_collection(c, entry.path());
      } else if (entry.is_regular_file()) {
        TagLib::FileStream fstream(entry.path().c_str(), true);
        TagLib::FileRef f(&fstream);

        if (f.isNull()) { continue; }

        TagLib::Tag* tag = f.tag();
        TagLib::AudioProperties* audio_props = f.audioProperties();
        TagLib::PropertyMap properties = f.file()->properties();
        if (properties.contains("ALBUMARTIST")) {
          TagLib::StringList artistList = properties["ALBUMARTIST"];

          // PropertyMap values are always lists (to support multiple values)
          for (const TagLib::String& artist : artistList) {
            std::cout << "Album Artist: " << artist.to8Bit(true) << std::endl;
          }
        } else {
          std::cout << "Album Artist tag not found." << std::endl;
        }

        size_t track_id = db::add_track((i32)tag->track(),
                                        utf8_to_utf32(tag->title().to8Bit(true)),
                                        utf8_to_utf32(tag->artist().to8Bit(true)),
                                        utf8_to_utf32("album artist"),
                                        utf8_to_utf32(tag->genre().to8Bit(true)),
                                        (i32)tag->year(),
                                        audio_props->bitrate(),
                                        audio_props->lengthInSeconds(),
                                        utf8_to_utf32(entry.path().string()));

        // track.collection_id = id;
        // track.track_number = (i32)tag->track();
        std::u32string album_title = utf8_to_utf32(tag->album().to8Bit(true));
        auto album = db::playlist_by_name(album_title);
        if (!album.has_value()) {
          size_t album_id = c->get().add_album(album_title, U"artist?");
          album = db::playlist_by_id(album_id);
        }
        if (album->get().image.empty()) {
          if (cover_art_from_image_in_directory.size() != 0) {
            album->get().image = cover_art_from_image_in_directory;
          } else {
            album->get().image = fetch_album_art(f);
          }
        }
        album->get().add_track(track_id);
      }
    }

    for (size_t playlist_id : c->get().playlist_ids) {
      db::playlist_by_id(playlist_id)->get().sort();
    }
  }

  inline void populate_collection(std::optional<std::reference_wrapper<db::Collection>> c, std::string_view path) {
    populate_collection(c, fs::path{path});
  }
} // namespace io
