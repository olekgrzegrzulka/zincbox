#pragma once
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "common/types.hpp"

namespace db {
  enum class PlaylistType : u8 {
    Album,
    User,
    Smart,
  };

  struct Playlist final {
    public:
      std::u32string name;
      std::u32string album_path;
      std::u32string author;
      std::vector<u8> image;
      PlaylistType type;
      bool tombstone = false;
      std::vector<size_t> track_ids;
      std::u32string cover_file_path;

    public:
      Playlist(std::ifstream&);
      Playlist(std::u32string_view name_, std::u32string_view author_, PlaylistType type_) {
        name = name_;
        author = author_;
        type = type_;
      }
      bool add_track(size_t);
      bool remove_track_by_id(size_t);
      bool remove_track_by_index(size_t);
      void sort_by_track_number();
      void set_tombstone(bool t) { tombstone = t; }
      bool is_tombstone() const { return tombstone; }

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
