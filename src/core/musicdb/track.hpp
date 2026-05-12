#pragma once
#include <string>
#include "common/types.hpp"

namespace db {
  struct Track final {
      Track(i32 track_number,
            std::u32string title,
            std::u32string artist,
            std::u32string album_artist,
            std::u32string genre,
            i32 year,
            i32 bitrate,
            i32 length_seconds,
            std::u32string path);

      Track(std::ifstream&);

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

    protected:
      bool tombstone = false;

    public:
      void serialize(std::ostream&) const;
      void set_tombstone(bool t) { tombstone = t; }
      bool is_tombstone() const { return tombstone; }

      bool operator==(const Track&) const = default;
  };
} // namespace db
