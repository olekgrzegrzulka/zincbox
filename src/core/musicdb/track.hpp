#pragma once
#include <bitset>
#include <optional>
#include <span>
#include <string>
#include "common/types.hpp"
#include "core/musicdb/types.hpp"

namespace db {

  enum TrackFlag : u8 { TOMBSTONE = 1, NOT_FOUND_DURING_RESCAN = 2, PLAYBACK_ERROR = 3, MAX = 4 };

  struct Track final {
      using enum TrackFlag;

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
      size_t originating_album_id = db::INVALID_ID;

    protected:
      std::bitset<TrackFlag::MAX> flags;

    public:
      void serialize(std::ostream&,
                     std::optional<std::span<size_t>> old_playlist_id_to_new_playlist_id = std::nullopt) const;
      std::string to_string() const;
      std::u32string pretty_name() const;
      std::u32string pretty_length() const;

      void set_tombstone(bool t) { flags.set(TOMBSTONE, t); }
      bool is_tombstone() const { return flags.test(TOMBSTONE); }
      void set_not_found_during_rescan(bool t) { flags.set(NOT_FOUND_DURING_RESCAN, t); }
      bool is_not_found_during_rescan() const { return flags.test(NOT_FOUND_DURING_RESCAN); }
      void set_playback_error(bool t) { flags.set(PLAYBACK_ERROR, t); }
      bool is_playback_error() const { return flags.test(PLAYBACK_ERROR); }

      bool operator==(const Track&) const = default;
  };
} // namespace db
