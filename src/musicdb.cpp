#include "musicdb.hpp"
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <taglib/fileref.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tstring.h>
#include "../lib/stb_image.h"
#include "../lib/stb_image_resize2.h"
#include "debug.hpp"
#include "flac/flacfile.h"
#include "mp4/mp4file.h"
#include "mpeg/id3v2/frames/attachedpictureframe.h"
#include "mpeg/id3v2/id3v2tag.h"
#include "ogg/vorbis/vorbisfile.h"
#include "ogg/xiphcomment.h"
#include "tfilestream.h"
#include "types.hpp"

namespace fs = std::filesystem;
using namespace musicdb;

// main data structure: hashmap for quick access
std::vector<Album> albums;

std::vector<u8> resize_album_art_to_64x64(const std::vector<u8>& raw_data) {
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

std::vector<u8> musicdb::fetch_album_art(TagLib::FileRef& ref) {
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

void scan_directory(std::string path) {
  // scan for cover art
  std::vector<u8> cover_art_from_image_in_directory(64 * 64 * 4);
  for (const auto& entry : fs::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      auto ext = entry.path().extension();
      bool is_img = (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" || ext == ".jpeg" || ext == ".JPEG" || ext == ".bmp" || ext == ".BMP" || ext == ".webp" || ext == ".WEBP");
      if (is_img) {
        i32 width, height, channels;
        stbi_uc* data = stbi_load(entry.path().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) { continue; }
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
      scan_directory(entry.path());
    } else if (entry.is_regular_file()) {
      TagLib::FileStream fstream(entry.path().c_str(), true);
      TagLib::FileRef f(&fstream);

      if (f.isNull()) { continue; }

      TagLib::Tag* tag = f.tag();
      TagLib::AudioProperties* audio_props = f.audioProperties();

      Track track{
        .track = (i32)tag->track(),
        .title = tag->title().toWString(),
        .artist = tag->artist().toWString(),
        .comment = tag->comment().toWString(),
        .genre = tag->genre().toWString(),
        .year = (i32)tag->year(),
        .bitrate = audio_props->bitrate(),
        .length_seconds = audio_props->lengthInSeconds(),
        .path = entry.path().string(),
      };

      std::wstring album_title = tag->album().toWString();
      Album* album = get_album_by_title(album_title);

      if (!album) {
        std::vector<u8> cover_art = fetch_album_art(f);
        if (cover_art.empty()) {
          cover_art = cover_art_from_image_in_directory;
        }
        i32 id = albums.size();
        Album a{
          .id = id,
          .title = album_title,
          .cover_art = cover_art,
          .tracks = {track},
        };
        add_album(std::move(a));
      } else {
        album->tracks.emplace(track);
      }
    }
  }
}

Album* musicdb::get_album_by_title(const std::wstring& title) {
  for (auto& album : albums) {
    if (album.title == title) {
      return &album;
    }
  }

  return nullptr;
}

void musicdb::load(std::string path) {
  clear_albums();

  if (!fs::exists(path)) {
    std::cerr << "Path does not exist: " << path << std::endl;
    return;
  }

  auto s = ScopeTimer{"scan_directory"};
  scan_directory(path);
}

void musicdb::load_from_path(std::string path) {
  int i = 0;
  for (const auto& entry : fs::recursive_directory_iterator(path)) {
    auto ext = entry.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    bool valid_ext = ext == ".mp3" || ext == ".m4a" || ext == ".aiff" || ext == ".wma" || ext == ".ogg" || ext == ".wav" || ext == ".flac" || ext == ".oga";
    if (entry.is_regular_file() && valid_ext) {
      debug_log("File: ", entry.path().string());
      i += 1;
    }
  }
  debug_log(i);
}

const std::vector<Album>& musicdb::get_albums() {
  return albums;
}

void musicdb::clear_albums() {
  albums.clear();
}

void musicdb::add_album(Album a) {
  std::stringstream ss;
  a.id = albums.size();
  albums.emplace_back(a);
}
