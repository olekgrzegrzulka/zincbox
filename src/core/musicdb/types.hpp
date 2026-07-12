#pragma once
#include <compare>
#include <cstddef>
#include <limits>

namespace db {
  static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

  using collection_id_t = size_t;
  using playlist_id_t = size_t;
  using track_id_t = size_t;

  struct track_info {
      size_t collection_id = INVALID_ID;
      size_t playlist_id = INVALID_ID;
      size_t track_id = INVALID_ID;
      size_t index = INVALID_ID;
      std::strong_ordering operator<=>(const track_info&) const = default;
  };

  struct playlist_info {
      size_t collection_id;
      size_t playlist_id;
  };
} // namespace db
