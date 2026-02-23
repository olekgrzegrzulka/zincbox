#include "musicdb.hpp"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
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

std::vector<Album> albums;
std::vector<Track> tracks;

std::vector<size_t> album_indices_sorted_by_name;

i32 musicdb::Album::length_seconds() const {
  i32 ret = 0;
  for (track_id_t id : track_ids) {
    ret += tracks[id].length_seconds;
  }
  return ret;
}

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

      Track track{};
      track.track_number = (i32)tag->track();
      track.title = tag->title().toWString();
      track.artist = tag->artist().toWString();
      track.genre = tag->genre().toWString();
      track.year = (i32)tag->year();
      track.bitrate = audio_props->bitrate();
      track.length_seconds = audio_props->lengthInSeconds();
      track.path = entry.path().string();

      std::wstring album_title = tag->album().toWString();
      Album* album = get_album_by_title(album_title);

      if (!album) {
        std::vector<u8> cover_art = fetch_album_art(f);
        if (cover_art.empty()) {
          cover_art = cover_art_from_image_in_directory;
        }
        i32 id = albums.size();
        track.album_id = id;
        track.track_id = 0;
        Album a{
          .title = album_title,
          .cover_art = cover_art,
        };
        album_id_t album_id = add_album(std::move(a));
        add_track(track, album_id);
      } else {
        add_track(track, album->album_id);
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

  {
    auto s = ScopeTimer{"scan_directory"};
    scan_directory(path);
  }

  {
    auto s = ScopeTimer{"sort_albums_and_tracks"};
    std::sort(album_indices_sorted_by_name.begin(), album_indices_sorted_by_name.end(), [](album_id_t lhs, album_id_t rhs) {
      return albums[lhs].title < albums[rhs].title;
    });

    for (album_id_t album_id = 1; album_id < albums.size() - 1; album_id += 1) {
      albums[album_indices_sorted_by_name[album_id]].next_album_id = album_indices_sorted_by_name[album_id + 1];
      albums[album_indices_sorted_by_name[album_id]].prev_album_id = album_indices_sorted_by_name[album_id - 1];
    }

    if (albums.size() > 0) {
      albums[album_indices_sorted_by_name.front()].prev_album_id = album_indices_sorted_by_name.back();
      albums[album_indices_sorted_by_name.back()].next_album_id = album_indices_sorted_by_name.front();
    }

    if (albums.size() >= 2) {
      albums[album_indices_sorted_by_name.front()].next_album_id = album_indices_sorted_by_name[1];
      albums[album_indices_sorted_by_name.back()].prev_album_id = album_indices_sorted_by_name[album_indices_sorted_by_name.size() - 2];
    }

    for (auto& a : albums) {
      std::sort(a.track_ids.begin(), a.track_ids.end(), [](track_id_t lhs, track_id_t rhs) {
        return tracks[lhs].track_number < tracks[rhs].track_number;
      });

      for (size_t i = 1; i < a.track_ids.size() - 1; i += 1) {
        tracks[a.track_ids[i]].next_track_id = a.track_ids[i + 1];
        tracks[a.track_ids[i]].prev_track_id = a.track_ids[i - 1];
      }

      a.first_track_id = a.track_ids.front();
      a.last_track_id = a.track_ids.back();
    }

    for (auto& a : albums) {
      tracks[a.first_track_id].prev_track_id = albums[a.prev_album_id].last_track_id;
      tracks[a.last_track_id].next_track_id = albums[a.next_album_id].first_track_id;

      if (a.track_ids.size() >= 2) {
        tracks[a.first_track_id].next_track_id = a.track_ids[1];
        tracks[a.last_track_id].prev_track_id = a.track_ids[a.track_ids.size() - 2];
      }
    }
  }

  check_integrity();
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

const Album& musicdb::get_album(album_id_t id) {
  return albums[id];
}

const std::vector<Album>& musicdb::get_albums() {
  return albums;
}

const std::vector<album_id_t>& musicdb::get_albums_sorted_by_name() {
  return album_indices_sorted_by_name;
}

const std::vector<Track>& musicdb::get_tracks() {
  return tracks;
}

void musicdb::clear_albums() {
  albums.clear();
}

track_id_t musicdb::add_track(Track t, album_id_t album_id) {
  t.track_id = tracks.size();
  t.album_id = album_id;
  tracks.emplace_back(t);
  albums[album_id].track_ids.emplace_back(t.track_id);

  // sort album.tracks by track_number
  // auto& album = albums[album_id];
  // auto& album_track_ids = album.track_ids;

  // auto it = std::lower_bound(album_track_ids.begin(), album_track_ids.end(), t.track_number,
  //                            [&](track_id_t lhs, track_id_t rhs) -> bool {
  //                              return tracks[album_track_ids[lhs]].track_number < tracks[album_track_ids[rhs]].track_number;
  //                            });
  // it = album_track_ids.insert(it, t.track_number);

  // // update next_album_id and prev_album_id
  // if (std::next(it) != album_track_ids.end()) {
  //   tracks[*it].next_track_id = album_indices_sorted_by_name[*std::next(it)];
  //   albums[album_indices_sorted_by_name[*std::next(it)]].prev_album_id = album_indices_sorted_by_name[*it];
  // } else {
  //   albums[album_indices_sorted_by_name[*it]].next_album_id = album_indices_sorted_by_name[0];
  // }

  // if (std::prev(it) != album_indices_sorted_by_name.end()) {
  //   albums[album_indices_sorted_by_name[*it]].prev_album_id = album_indices_sorted_by_name[*std::prev(it)];
  //   albums[album_indices_sorted_by_name[*std::prev(it)]].prev_album_id = album_indices_sorted_by_name[*it];
  // } else {
  //   albums[album_indices_sorted_by_name[*it]].prev_album_id = album_indices_sorted_by_name[album_indices_sorted_by_name.size() - 1];
  // }

  // albums[album_indices_sorted_by_name[0]].prev_album_id = album_indices_sorted_by_name[album_indices_sorted_by_name.size() - 1];

  // album.first_track_id = album.track_ids[0];

  // check_integrity();
  return tracks.size() - 1;
}

album_id_t musicdb::add_album(Album a) {
  a.album_id = albums.size();
  albums.emplace_back(a);
  album_indices_sorted_by_name.emplace_back(a.album_id);

  // sort album_indices_sorted_by_name
  // album_indices_sorted_by_name.emplace_back(a.album_id);
  // auto it = std::lower_bound(album_indices_sorted_by_name.begin(), album_indices_sorted_by_name.end(), album_indices_sorted_by_name.size() - 1,
  //                            [&](size_t lhs, size_t rhs) -> bool { return albums[lhs].title < albums[rhs].title; });
  // album_indices_sorted_by_name.pop_back();
  // it = album_indices_sorted_by_name.insert(it, a.album_id);

  // // update next_album_id and prev_album_id
  // if (std::next(it) != album_indices_sorted_by_name.end()) {
  //   albums[album_indices_sorted_by_name[*it]].next_album_id = album_indices_sorted_by_name[*std::next(it)];
  //   albums[album_indices_sorted_by_name[*std::next(it)]].prev_album_id = album_indices_sorted_by_name[*it];
  // } else {
  //   albums[album_indices_sorted_by_name[*it]].next_album_id = album_indices_sorted_by_name[0];
  // }

  // if (std::prev(it) != album_indices_sorted_by_name.end()) {
  //   albums[album_indices_sorted_by_name[*it]].prev_album_id = album_indices_sorted_by_name[*std::prev(it)];
  //   albums[album_indices_sorted_by_name[*std::prev(it)]].prev_album_id = album_indices_sorted_by_name[*it];
  // } else {
  //   albums[album_indices_sorted_by_name[*it]].prev_album_id = album_indices_sorted_by_name[album_indices_sorted_by_name.size() - 1];
  // }

  // albums[album_indices_sorted_by_name[0]].prev_album_id = album_indices_sorted_by_name[album_indices_sorted_by_name.size() - 1];
  // check_integrity();
  return albums.size() - 1;
}

void musicdb::check_integrity() {
  static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

  debug_log("musicdb::check_integrity()");

  ensure(album_indices_sorted_by_name.size() == albums.size());

  if (albums.size() > 0) {
    size_t album_id_first = album_indices_sorted_by_name[0];
    size_t album_id_last = album_indices_sorted_by_name[albums.size() - 1];
    ensure(albums[album_id_first].prev_album_id == album_id_last);
    ensure(albums[album_id_last].next_album_id == album_id_first);
  }

  for (size_t i = 1; i < album_indices_sorted_by_name.size() - 1; i += 1) {
    size_t album_id_curr = album_indices_sorted_by_name[i];
    size_t album_id_prev = album_indices_sorted_by_name[i - 1];
    size_t album_id_next = album_indices_sorted_by_name[i + 1];

    ensure(album_id_curr >= 0 && album_id_curr < albums.size());
    ensure(album_id_prev >= 0 && album_id_prev < albums.size());
    ensure(album_id_next >= 0 && album_id_next < albums.size());

    ensure(albums[album_id_curr].next_album_id == album_id_next);
    ensure(albums[album_id_curr].prev_album_id == album_id_prev);

    // FIXME: we assume that all albums are non-empty
    if (albums[album_id_curr].track_ids.size() > 0 && albums[album_id_next].track_ids.size() > 0) {
      ensure(tracks[albums[album_id_curr].last_track_id].next_track_id == albums[album_id_next].first_track_id);
    }
    if (albums[album_id_curr].track_ids.size() > 0 && albums[album_id_prev].track_ids.size() > 0) {
      ensure(tracks[albums[album_id_curr].first_track_id].prev_track_id == albums[album_id_prev].last_track_id);
    }
  }

  if (tracks.size() > 0) {
    size_t album_id_first = album_indices_sorted_by_name[0];
    size_t album_id_last = album_indices_sorted_by_name[albums.size() - 1];

    ensure(album_id_first >= 0 && album_id_first < albums.size());
    ensure(album_id_last >= 0 && album_id_last < albums.size());

    ensure(albums[album_id_first].prev_album_id == album_id_last);
    ensure(albums[album_id_last].next_album_id == album_id_first);
  }

  for (size_t i = 1; i < tracks.size() - 1; i += 1) {
    ensure(tracks[i].album_id != INVALID_ID);
    auto& album = albums[tracks[i].album_id];
    ensure(album.album_id == tracks[i].album_id);
    bool album_contains_track = std::any_of(album.track_ids.begin(),
                                            album.track_ids.end(),
                                            [&](track_id_t track_id) { return tracks[i].track_id == track_id; });
    ensure(album_contains_track);

    size_t track_id_prev = tracks[i].prev_track_id;
    size_t track_id_next = tracks[i].next_track_id;

    ensure(track_id_prev >= 0 && track_id_prev < tracks.size());
    ensure(track_id_next >= 0 && track_id_next < tracks.size());

    ensure(tracks[i].next_track_id == track_id_next);
    ensure(tracks[i].prev_track_id == track_id_prev);
  }
}
