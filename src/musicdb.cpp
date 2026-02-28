#include "musicdb.hpp"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
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

std::vector<Collection> collections{};

void Collection::build() {
  auto s = ScopeTimer{"sort_albums_and_tracks"};
  ensure(album_ids_by_name.size() == albums.size());
  if (albums.size() == 0) { return; }
  std::sort(album_ids_by_name.begin(), album_ids_by_name.end(), [this](album_id_t lhs, album_id_t rhs) {
    return albums[lhs].title < albums[rhs].title;
  });

  for (album_id_t album_id = 1; album_id < albums.size() - 1; album_id += 1) {
    albums[album_ids_by_name[album_id]].next_album_id = album_ids_by_name[album_id + 1];
    albums[album_ids_by_name[album_id]].prev_album_id = album_ids_by_name[album_id - 1];
  }

  albums[album_ids_by_name.front()].prev_album_id = album_ids_by_name.back();
  albums[album_ids_by_name.back()].next_album_id = album_ids_by_name.front();

  if (albums.size() >= 2) {
    albums[album_ids_by_name.front()].next_album_id = album_ids_by_name[1];
    albums[album_ids_by_name.back()].prev_album_id = album_ids_by_name[album_ids_by_name.size() - 2];
  }

  for (auto& a : albums) {
    std::sort(a.track_ids.begin(), a.track_ids.end(), [this](track_id_t lhs, track_id_t rhs) {
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

template <typename T>
void write_bin(std::ostream& os, const T& value) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void write_str(std::ostream& os, const std::basic_string<T>& s) {
  uint64_t count = s.size();
  write_bin(os, count);
  os.write(reinterpret_cast<const char*>(s.data()), count * sizeof(T));
}

void write_blob(std::ostream& os, const std::vector<u8>& v) {
  uint64_t size = v.size();
  write_bin(os, size);
  os.write(reinterpret_cast<const char*>(v.data()), size);
}

template <typename T>
void read_bin(std::istream& is, T& value) {
  is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template <typename T>
void read_str(std::istream& is, std::basic_string<T>& s) {
  uint64_t count;
  read_bin(is, count);
  s.resize(count);
  if (count > 0) {
    is.read(reinterpret_cast<char*>(&s[0]), count * sizeof(T));
  }
}

void read_blob(std::istream& is, std::vector<uint8_t>& v) {
  uint64_t size;
  read_bin(is, size);
  v.resize(size);
  is.read(reinterpret_cast<char*>(v.data()), size);
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

std::vector<u8> fetch_album_art(const TagLib::FileRef& ref) {
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

collection_id_t musicdb::add_collection(std::string_view name) {
  collections.emplace_back(Collection{name, collections.size()});
  return collections.size() - 1;
}

std::vector<Collection>& musicdb::get_collections() {
  return collections;
}

const Track* musicdb::get_track(collection_id_t c, track_id_t id) {
  return get_collections()[c].get_track(id);
}
const Album* musicdb::get_album(collection_id_t c, album_id_t id) {
  return get_collections()[c].get_album(id);
}

const Album* musicdb::get_next_album(collection_id_t c, album_id_t id) {
  const Album* a = get_album(c, id);
  return get_collections()[c].get_album(a->next_album_id);
}

Collection* musicdb::get_collection(u64 i) {
  return &collections[i];
}

Collection* musicdb::get_collection(std::string_view sv) {
  for (auto& c : collections) {
    if (c == sv) {
      return &c;
    }
  }
  return nullptr;
}

void musicdb::load_collections_from_file(std::ifstream& is) {
  collections.clear();

  auto s = ScopeTimer{"load_collections_from_file"};

  u64 collection_count = 0;
  read_bin(is, collection_count);

  for (u64 c = 0; c < collection_count; c += 1) {
    std::string name;
    auto collection = Collection{is, collections.size()};
    if (collection.get_albums().size() > 0) {
      collections.emplace_back(std::move(collection));
    }
  }
}

void musicdb::save_collections_to_file(std::ofstream& os) {
  auto s = ScopeTimer{"save_to_file"};

  write_bin(os, collections.size());

  for (auto& c : collections) {
    write_str(os, c.get_name());
    c.save_to_file(os);
  }
}

Collection::Collection(std::string_view sv, collection_id_t id_) {
  id = id_;
  name = std::string{sv};
}

Collection::Collection(std::ifstream& is, collection_id_t id_) {
  id = id_;
  read_str(is, name);
  u64 album_count = 0;
  read_bin(is, album_count);

  for (u64 i = 0; i < album_count; ++i) {
    Album album;
    album.collection_id = id;
    read_str(is, album.title);
    read_bin(is, album.album_id);
    read_bin(is, album.prev_album_id);
    read_bin(is, album.next_album_id);
    read_bin(is, album.first_track_id);
    read_bin(is, album.last_track_id);
    read_blob(is, album.cover_art);

    add_album(std::move(album));

    u64 track_count;
    read_bin(is, track_count);

    for (u64 j = 0; j < track_count; ++j) {
      Track t;
      t.collection_id = id;
      read_bin(is, t.track_id);
      read_bin(is, t.prev_track_id);
      read_bin(is, t.next_track_id);
      read_bin(is, t.album_id);
      read_bin(is, t.track_number);
      read_str(is, t.title);
      read_str(is, t.artist);
      read_str(is, t.genre);
      read_bin(is, t.year);
      read_bin(is, t.bitrate);
      read_bin(is, t.length_seconds);
      read_str(is, t.path);
      add_track(std::move(t), t.album_id);
    }

    // sort_albums_and_tracks(collection.albums, collection.album_ids_by_name, collection.tracks);
    // collection.check_integrity();
  }
}

void Collection::save_to_file(std::ofstream& os) {
  u64 album_count = get_albums().size();
  write_bin(os, album_count);

  for (auto& album : get_albums()) {
    write_str(os, album.title);
    write_bin(os, album.album_id);
    write_bin(os, album.prev_album_id);
    write_bin(os, album.next_album_id);
    write_bin(os, album.first_track_id);
    write_bin(os, album.last_track_id);
    write_blob(os, album.cover_art);

    size_t track_count = album.track_ids.size();
    write_bin(os, track_count);

    for (track_id_t track_id : album.track_ids) {
      auto& t = get_tracks()[track_id];

      write_bin(os, t.track_id);
      write_bin(os, t.prev_track_id);
      write_bin(os, t.next_track_id);
      write_bin(os, t.album_id);
      write_bin(os, t.track_number);
      write_str(os, t.title);
      write_str(os, t.artist);
      write_str(os, t.genre);
      write_bin(os, t.year);
      write_bin(os, t.bitrate);
      write_bin(os, t.length_seconds);
      write_str(os, t.path);
    }
  }
}

void Collection::scan_directory(fs::path path) {
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
      track.collection_id = id;
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
        album_id_t album_id = albums.size();
        track.album_id = album_id;
        track.track_id = 0;
        Album a{
          .collection_id = id,
          .title = album_title,
          .cover_art = cover_art,
        };
        album_id_t album_id_ = add_album(std::move(a));
        ensure(album_id == album_id_);
        add_track(track, album_id_);
      } else {
        add_track(track, album->album_id);
      }
    }
  }
}

Album* Collection::get_album_by_title(const std::wstring& title) {
  for (auto& album : albums) {
    if (album.title == title) {
      return &album;
    }
  }

  return nullptr;
}

bool Collection::add_path(std::string_view path) {
  if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
    return false;
  }

  if (!fs::exists(path)) {
    return false;
  }

  {
    auto s = ScopeTimer{"scan_directory"};
    scan_directory(path);
  }

  build();
  check_integrity();
  return true;
}

const Album* Collection::get_album(album_id_t album_id) {
  if (!is_built) {
    build();
    is_built = true;
  }
  return &albums[album_id];
}

const std::vector<Album>& Collection::get_albums() {
  if (!is_built) {
    build();
    is_built = true;
  }
  return albums;
}

const std::vector<album_id_t>& Collection::get_albums_sorted_by_name() {
  if (!is_built) {
    build();
    is_built = true;
  }
  return album_ids_by_name;
}

const Track* Collection::get_track(track_id_t track_id) {
  if (!is_built) {
    build();
    is_built = true;
  }
  return &tracks[track_id];
}

const std::vector<Track>& Collection::get_tracks() {
  if (!is_built) {
    build();
    is_built = true;
  }
  return tracks;
}

track_id_t Collection::add_track(Track t, album_id_t album_id) {
  t.track_id = tracks.size();
  t.album_id = album_id;
  tracks.emplace_back(t);
  ensure(albums.size() > album_id);
  albums[album_id].track_ids.emplace_back(t.track_id);

  return tracks.size() - 1;
}

album_id_t Collection::add_album(Album a) {
  a.album_id = albums.size();
  albums.emplace_back(a);
  album_ids_by_name.emplace_back(a.album_id);
  return albums.size() - 1;
}

void Collection::check_integrity() {
  static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

  debug_log("musicdb::check_integrity()");

  ensure(album_ids_by_name.size() == albums.size());

  if (albums.size() > 0) {
    size_t album_id_first = album_ids_by_name[0];
    size_t album_id_last = album_ids_by_name[albums.size() - 1];
    ensure(albums[album_id_first].prev_album_id == album_id_last);
    ensure(albums[album_id_last].next_album_id == album_id_first);
  }
  for (i32 i = 1; i < (i32)album_ids_by_name.size() - 1; i += 1) {
    size_t album_id_curr = album_ids_by_name[i];
    size_t album_id_prev = album_ids_by_name[i - 1];
    size_t album_id_next = album_ids_by_name[i + 1];

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
    size_t album_id_first = album_ids_by_name[0];
    size_t album_id_last = album_ids_by_name[albums.size() - 1];

    ensure(album_id_first >= 0 && album_id_first < albums.size());
    ensure(album_id_last >= 0 && album_id_last < albums.size());

    ensure(albums[album_id_first].prev_album_id == album_id_last);
    ensure(albums[album_id_last].next_album_id == album_id_first);
  }
  for (i32 i = 1; i < (i32)tracks.size() - 1; i += 1) {
    ensure(tracks[i].album_id != INVALID_ID);
    auto& album = albums[tracks[i].album_id];
    ensure(album.album_id == tracks[i].album_id);
    bool album_contains_track = std::any_of(album.track_ids.begin(),
                                            album.track_ids.end(),
                                            [&](track_id_t track_id) { return tracks[i].track_id == track_id; });
    if (!album_contains_track) {

      debug_log(tracks[i].track_id);
      debug_log("-----------");
      debug_log(album.title);
      debug_log(album.track_ids);
      debug_log(album.track_ids.size());
    }
    ensure(album_contains_track);

    size_t track_id_prev = tracks[i].prev_track_id;
    size_t track_id_next = tracks[i].next_track_id;

    ensure(track_id_prev >= 0 && track_id_prev < tracks.size());
    ensure(track_id_next >= 0 && track_id_next < tracks.size());

    ensure(tracks[i].next_track_id == track_id_next);
    ensure(tracks[i].prev_track_id == track_id_prev);
  }
}
