#pragma once
#include <optional>
#include <string>
#include <vector>
#include "common/types.hpp"

namespace zincbox {
  struct TrackMetadata final {
      std::string title;
      std::string artist;
      std::string album;
      std::vector<std::string> album_artist;
      std::string genre;
      std::optional<i32> track_number;
      std::optional<i32> track_total;
      std::optional<i32> year;
      i32 duration_ms{0};
      i32 bitrate_kb_s{0};

      TrackMetadata() = default;
      TrackMetadata(const TrackMetadata&) = delete;
      TrackMetadata& operator=(const TrackMetadata&) = delete;
      TrackMetadata(TrackMetadata&&) noexcept = default;
      TrackMetadata& operator=(TrackMetadata&&) noexcept = default;
      bool operator==(const TrackMetadata&) const = default;
  };
} // namespace zincbox
