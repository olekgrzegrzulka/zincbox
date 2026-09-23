#pragma once
#include <bitset>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <stddef.h>
#include "common/types.hpp"
#include "core/musicdb/track_metadata.hpp"
#include "core/musicdb/types.hpp"

namespace db {

  namespace fs = std::filesystem;

  enum TrackFlag : u8 {
    ORPHANED = 1,
    NEW = 2,
    TOMBSTONE = 3,
    NOT_FOUND_DURING_RESCAN = 4,
    PLAYBACK_ERROR = 5,
    MAX = 16
  };

  struct Track final {
      using enum TrackFlag;

      Track();
      Track(std::ifstream&);

      zincbox::TrackMetadata metadata;
      std::bitset<TrackFlag::MAX> flags;
      fs::path file_path;
      fs::path file_name_with_extension() const { return file_path.filename(); }
      fs::path file_name_without_extension() const { return file_path.stem(); }
      fs::path file_extension() const { return file_path.extension(); }
      fs::path parent_directory_name() const { return file_path.parent_path().stem(); }
      std::string hash{};
      size_t originating_album_id = db::INVALID_ID;

    public:
      void serialize(std::ostream&,
                     std::optional<std::span<size_t>> old_playlist_id_to_new_playlist_id = std::nullopt) const;
      std::string to_string() const;
      std::string pretty_title() const;
      std::string pretty_name() const;
      std::string pretty_length() const;
      std::string pretty_album_artist() const;

      void set_flag(TrackFlag flag) { flags.set(flag, true); }
      void unset_flag(TrackFlag flag) { flags.set(flag, false); }
      bool get_flag(TrackFlag flag) const { return flags.test(flag); }

      bool operator==(const Track&) const = default;
  };
} // namespace db
