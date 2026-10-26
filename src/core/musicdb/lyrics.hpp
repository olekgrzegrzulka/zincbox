#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include "common/types.hpp"

namespace zincbox {

  struct LyricLine {
      i32 timestamp_ms{};
      std::string text;

      bool operator==(const LyricLine&) const = default;
  };

  struct Lyrics {
      std::string unsynced;
      std::vector<LyricLine> synced;

      bool empty() const { return unsynced.empty() && synced.empty(); }
      bool is_synced() const { return !synced.empty(); }

      std::optional<size_t> get_active_line_index(i32 playback_ms) const {
        if (synced.empty()) { return std::nullopt; }

        auto it = std::upper_bound(synced.begin(), synced.end(), playback_ms,
                                   [](i32 time, const LyricLine& line) -> bool { return time < line.timestamp_ms; });

        if (it == synced.begin()) { return std::nullopt; }
        return static_cast<size_t>(std::distance(synced.begin(), std::prev(it)));
      }

      bool operator==(const Lyrics&) const = default;
  };

} // namespace zincbox
