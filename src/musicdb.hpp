#pragma once
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "types.hpp"

namespace TagLib {
  class FileRef;
}

namespace musicdb {
  using track_id_t = size_t;
  using album_id_t = size_t;

  struct Track {
    track_id_t track_id = std::numeric_limits<size_t>::max();
    track_id_t prev_track_id = std::numeric_limits<size_t>::max();
    track_id_t next_track_id = std::numeric_limits<size_t>::max();

    album_id_t album_id = std::numeric_limits<size_t>::max();

    i32 track_number;
    std::wstring title;
    std::wstring artist;
    std::wstring genre;
    i32 year;
    i32 bitrate;
    i32 length_seconds;
    std::string path;
  };

  struct Album {
    album_id_t album_id = std::numeric_limits<size_t>::max();
    album_id_t prev_album_id = std::numeric_limits<size_t>::max();
    album_id_t next_album_id = std::numeric_limits<size_t>::max();

    track_id_t first_track_id = std::numeric_limits<size_t>::max();
    track_id_t last_track_id = std::numeric_limits<size_t>::max();

    std::wstring title;
    std::vector<uint8_t> cover_art;
    std::vector<track_id_t> track_ids;

    bool operator==(const Album& other) const {
      return title == other.title;
    }

    i32 length_seconds() const;
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

  std::vector<uint8_t> fetch_album_art(TagLib::FileRef& ref);

  void load(std::string path);

  void load_from_path(std::string path);

  const std::vector<Album>& get_albums();
  const Album& get_album(album_id_t);
  const std::vector<album_id_t>& get_albums_sorted_by_name();
  const std::vector<Track>& get_tracks();

  void clear_albums();

  track_id_t add_track(Track, album_id_t);
  album_id_t add_album(Album);

  Album* get_album_by_title(const std::wstring& title);

  // const Track* next_track(const Track*);

  void check_integrity();

} // namespace musicdb
