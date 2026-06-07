#pragma once
#include <bitset>
#include <string>
#include "common/types.hpp"

namespace db {

  enum TrackFlag : u8 {
    TOMBSTONE = 1,
    NOT_FOUND_DURING_RESCAN = 2,
    PLAYBACK_ERROR = 3,
    MAX = 4,
  };

  struct Track final {
      Track(i32 track_number, std::u32string title, std::u32string artist, std::u32string album_artist,
            std::u32string genre, i32 year, i32 bitrate, i32 length_seconds, std::u32string path);

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
      bool flag_not_found_during_rescan = false;

    protected:
      std::bitset<TrackFlag::MAX> flags;

    public:
      void serialize(std::ostream&) const;
      std::string to_string() const;

      void set_tombstone(bool t) { flags.set(TrackFlag::TOMBSTONE, t); }
      bool is_tombstone() const { return flags.test(TrackFlag::TOMBSTONE); }
      void set_flag_not_found_during_rescan(bool t) { flags.set(TrackFlag::NOT_FOUND_DURING_RESCAN, t); }
      bool is_flag_not_found_during_rescan() const { return flags.test(TrackFlag::NOT_FOUND_DURING_RESCAN); }
      void set_playback_error(bool t) { flags.set(TrackFlag::PLAYBACK_ERROR, t); }
      bool is_playback_error() const { return flags.test(TrackFlag::PLAYBACK_ERROR); }

      bool operator==(const Track&) const = default;
  };
} // namespace db
