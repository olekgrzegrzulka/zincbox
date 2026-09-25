#pragma once
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "common/types.hpp"

namespace db {
  enum class PlaylistType : u8 { Album, User, Smart };

  struct Playlist final {
    public:
      std::string name;
      std::string album_path;
      std::vector<std::string> author;
      std::vector<u8> art_64x64;
      PlaylistType type;
      bool tombstone = false;
      std::vector<size_t> track_ids;
      std::string art_file_path;

    public:
      Playlist(std::ifstream&);
      Playlist(std::string_view name_, std::vector<std::string> author_, PlaylistType type_) {
        name = name_;
        author = std::move(author_);
        type = type_;
      }
      bool add_track(size_t);
      std::vector<size_t> insert_tracks(size_t at, std::span<const size_t>);
      bool remove_track_by_id(size_t);
      bool remove_track_by_index(size_t);
      void remove_tracks_by_indices(std::span<const size_t>);
      void sort_by_track_number();
      void sort_by_artist_asc();
      void sort_by_artist_desc();
      void sort_by_name_asc();
      void sort_by_name_desc();
      void set_tombstone(bool t) { tombstone = t; }
      bool is_tombstone() const { return tombstone; }
      std::string author_pretty() const;

      std::optional<size_t> next_track_id(size_t track_id) const;
      std::optional<size_t> prev_track_id(size_t track_id) const;
      bool has_track_id(size_t track_id) const;
      std::optional<size_t> find_track_index(size_t track_id) const;

      const std::vector<size_t>& get_track_ids() const { return track_ids; }
      size_t get_tracks_count() const { return track_ids.size(); }

      void serialize(std::ostream&) const;
      void serialize(std::ostream&, const std::vector<size_t>& old_track_id_to_new_track_id) const;
  };
} // namespace db
