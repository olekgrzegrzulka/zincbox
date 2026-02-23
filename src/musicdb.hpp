#pragma once
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include "types.hpp"

namespace TagLib {
  class FileRef;
}

namespace musicdb {

  struct Track {
    i32 track;
    std::wstring title;
    std::wstring artist;
    std::wstring comment;
    std::wstring genre;
    i32 year;
    i32 bitrate;
    i32 length_seconds;

    bool operator<(const Track& other) const {
      return track < other.track;
    }
  };

  struct Album {
    std::string id;
    std::wstring title;
    std::vector<uint8_t> cover_art;
    std::multiset<Track> tracks;

    bool operator==(const Album& other) const {
      return title == other.title;
    }

    i32 length_seconds() const {
      i32 ret = 0;
      for (auto& t : tracks) {
        ret += t.length_seconds;
      }
      return ret;
    }
  };

  struct AlbumComparer {
    bool operator()(const Album* lhs, const Album* rhs) const {
      return lhs->title < rhs->title;
    }
  };

  struct AlbumHash {
    using is_transparent = void;

    std::size_t operator()(const Album& album) const {
      return std::hash<std::wstring>{}(album.title);
    }
    std::size_t operator()(const std::unique_ptr<musicdb::Album>& album) const {
      return std::hash<std::wstring>{}(album->title);
    }
    std::size_t operator()(const std::wstring& title) const {
      return std::hash<std::wstring>{}(title);
    }
  };

  struct AlbumEq {
    using is_transparent = void;

    bool operator()(const Album& lhs, const Album& rhs) const { return lhs.title == rhs.title; }
    bool operator()(const std::unique_ptr<musicdb::Album>& lhs, const std::unique_ptr<musicdb::Album>& rhs) const { return lhs->title == rhs->title; }
    bool operator()(const Album& lhs, const std::unique_ptr<musicdb::Album>& rhs) const { return lhs.title == rhs->title; }
    bool operator()(const std::unique_ptr<musicdb::Album>& lhs, const Album& rhs) const { return lhs->title == rhs.title; }
    bool operator()(const Album& lhs, const std::wstring& rhs_title) const { return lhs.title == rhs_title; }
    bool operator()(const std::wstring& lhs_title, const Album& rhs) const { return lhs_title == rhs.title; }
  };

  void print_shit();

  std::vector<uint8_t> fetch_album_art(TagLib::FileRef& ref);

  void load(std::string path);

  void load_from_path(std::string path);

  const std::vector<Album>& get_albums();

  void clear_albums();

  void add_album(Album);

  Album* get_album_by_title(const std::wstring& title);

} // namespace musicdb