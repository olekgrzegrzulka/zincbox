#pragma once
#include <cstddef>
#include <limits>

namespace db {
  using collection_id_t = size_t;
  using playlist_id_t = size_t;
  using track_id_t = size_t;

  static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();
} // namespace db
