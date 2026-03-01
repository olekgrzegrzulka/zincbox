#pragma once
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "types.hpp"

namespace TagLib {
  class FileRef;
}

namespace musicdb {
  struct Track;
  struct Album;
  class Collection;
  class Playlist;
  using collection_id_t = size_t;
  using track_id_t = size_t;
  using album_id_t = size_t;
  using playlist_id_t = size_t;

  collection_id_t add_collection(std::string_view);
  void mark_collection_as_tombstone(collection_id_t);

  std::vector<Collection>& get_collections();
  const Track* get_track(collection_id_t, track_id_t);
  const Album* get_album(collection_id_t, album_id_t);
  const Album* get_next_album(collection_id_t, album_id_t);
  Collection* get_collection(u64);
  Collection* get_collection(std::string_view);

  void save_collections_to_file(std::ofstream&);
  void load_collections_from_file(std::ifstream&);

  Playlist* playlist(playlist_id_t);
  bool delete_playlist(playlist_id_t);
  playlist_id_t add_playlist(std::string_view);

  struct Track {
    track_id_t track_id = std::numeric_limits<size_t>::max();
    track_id_t prev_track_id = std::numeric_limits<size_t>::max();
    track_id_t next_track_id = std::numeric_limits<size_t>::max();

    album_id_t album_id = std::numeric_limits<size_t>::max();
    collection_id_t collection_id = std::numeric_limits<size_t>::max();

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
    collection_id_t collection_id = std::numeric_limits<size_t>::max();

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

  class Collection {
  protected:
    collection_id_t id = std::numeric_limits<size_t>::max();
    std::string name;
    std::vector<std::string> paths;
    std::vector<Album> albums;
    std::vector<Track> tracks;

    bool tombstone = false;
    bool is_built = false;
    std::vector<size_t> album_ids_by_name;

  public:
    Collection(std::string_view name, collection_id_t);
    Collection(std::ifstream&, collection_id_t);
    void save_to_file(std::ofstream&);

    bool add_path(std::string_view);

    bool operator==(std::string_view sv) const { return sv == name; }
    collection_id_t get_id() const { return id; }
    std::string get_name() const { return name; }
    const std::vector<Album>& get_albums();
    const std::vector<album_id_t>& get_albums_sorted_by_name();
    const Album* get_album(album_id_t);
    const Track* get_track(track_id_t);
    const std::vector<Track>& get_tracks();
    bool is_tombstone() const { return tombstone; }

    track_id_t add_track(Track, album_id_t);
    album_id_t add_album(Album);
    Album* get_album_by_title(const std::wstring& title);

    void mark_as_tombstone() { tombstone = true; }
    void check_integrity();

  protected:
    void scan_directory(std::filesystem::path);
    void build();
  };
  struct playlist_track final {
    friend class Playlist;

    friend bool operator==(const playlist_track& lhs, const playlist_track& rhs) {
      return lhs.track_id == rhs.track_id && lhs.album_id == rhs.album_id && lhs.collection_id == rhs.collection_id;
    }

  public:
    collection_id_t collection_id{};
    album_id_t album_id{};
    track_id_t track_id{};

  private:
    size_t playlist_index{};
  };

  class Playlist {
  public:
    Playlist(std::string_view name_, playlist_id_t id_) {
      name = name_;
      id = id_;
    }

  protected:
    playlist_id_t id = std::numeric_limits<size_t>::max();
    size_t first_track_index = 0;
    std::string name{};
    std::vector<playlist_track> tracks{};

  public:
    void add_track(collection_id_t, track_id_t);
    void sort_by_track_name_ascending();
    std::optional<playlist_track> get_next_track(playlist_track);

    playlist_id_t get_id() const { return id; }
    std::string_view get_name() const { return name; }

  protected:
    void remove_track_by_index(size_t);
  };
} // namespace musicdb
