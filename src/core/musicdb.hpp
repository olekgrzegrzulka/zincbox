#pragma once
#include <cstddef>
#include <fstream>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "../types.hpp"

namespace db {
  enum class PlaylistType {
    Album,
    User,
    Smart,
  };

  struct Track final {
      friend size_t add_track(i32, std::u32string, std::u32string, std::u32string, std::u32string, i32, i32, i32, std::u32string);
      friend size_t add_track(std::ifstream&);
      friend void deserialize(std::ifstream&);

    public:
      i32 track_number;
      std::u32string title;
      std::u32string artist;
      std::u32string album_artist;
      std::u32string genre;
      i32 year;
      i32 bitrate;
      i32 length_seconds;
      std::u32string path;

    public:
      void serialize(std::ostream&);
      bool operator==(const Track& rhs) const = default;
      bool is_tombstone() const { return false; }

    protected:
      Track() = default;
      Track(std::ifstream& is);
  };

  struct TrackHashByAlbumArtist {
      using is_transparent = void;

      std::size_t operator()(const Track& album) const {
        return std::hash<std::u32string>{}(album.album_artist);
      }
      std::size_t operator()(const std::u32string& album_artist) const {
        return std::hash<std::u32string>{}(album_artist);
      }
  };

  struct Collection {
      Collection(std::u32string_view name_) { name = name_; }
      Collection(std::ifstream&);
      std::u32string name;
      std::vector<size_t> playlist_ids;

      bool add_path(std::string);
      size_t add_playlist(std::u32string_view title, std::u32string_view artist);
      size_t add_playlist(std::ifstream&);
      size_t add_album(std::u32string_view title, std::u32string_view artist);
      std::optional<size_t> next_playlist_id(size_t playlist_id);
      std::optional<size_t> prev_playlist_id(size_t playlist_id);
      bool is_tombstone() const { return false; }
      void serialize(std::ofstream&);

    protected:
      std::unordered_set<std::string> paths;
      std::optional<size_t> find_playlist_index(size_t playlist_id);
  };

  struct Playlist {
      friend size_t Collection::add_album(std::u32string_view, std::u32string_view);
      friend size_t Collection::add_playlist(std::u32string_view, std::u32string_view);
      friend size_t Collection::add_playlist(std::ifstream&);
      friend void deserialize(std::ifstream&);

      std::u32string name;
      std::u32string author;
      std::vector<uint8_t> image;
      PlaylistType type;

      bool add_track(size_t);
      bool remove_track_by_id(size_t);
      bool remove_track_by_index(size_t);
      void sort();
      bool is_tombstone() const { return false; }

      std::optional<size_t> next_track_id(size_t track_id);
      std::optional<size_t> prev_track_id(size_t track_id);
      bool has_track_id(size_t track_id);
      std::optional<size_t> find_track_index(size_t track_id);

      const std::vector<size_t>& get_track_ids() const { return track_ids; }
      size_t get_tracks_count() const { return track_ids.size(); }
      bool emplace_track_id(size_t track_id) {
        if (std::find(track_ids.begin(), track_ids.end(), track_id) == track_ids.end()) {
          track_ids.emplace_back(track_id);
          return true;
        }
        return false;
      }

      void serialize(std::ostream&);

    protected:
      std::vector<size_t> track_ids;

      Playlist(std::ifstream&);
      Playlist(std::u32string_view name_, std::u32string_view author_, PlaylistType type_) {
        name = name_;
        author = author_;
        type = type_;
      }
  };

  size_t add_collection(std::u32string_view);
  std::optional<std::reference_wrapper<Collection>> collection_by_id(size_t);
  const std::vector<Collection>& all_collections();
  size_t collection_count();

  std::optional<std::reference_wrapper<Playlist>> playlist_by_id(size_t);
  std::optional<std::reference_wrapper<Playlist>> playlist_by_name(std::u32string_view);
  const std::vector<Playlist>& all_playlists();
  size_t playlist_count();

  std::optional<std::reference_wrapper<const Track>> track_by_id(size_t);
  std::optional<size_t> track_by_title(std::u32string_view);
  const std::vector<Track>& all_tracks();
  size_t track_count();

  size_t add_track(i32 track_number,
                   std::u32string title,
                   std::u32string artist,
                   std::u32string album_artist,
                   std::u32string genre,
                   i32 year,
                   i32 bitrate,
                   i32 length_seconds,
                   std::u32string path);
  size_t add_track(std::ifstream&);

  void serialize(std::ofstream&);
  void deserialize(std::ifstream&);

  void print_collections();
} // namespace db
