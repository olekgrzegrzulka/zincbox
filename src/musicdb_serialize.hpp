#pragma once
#include <string>
#include <unordered_set>
#include "musicdb.hpp"

namespace musicdb {

  // --- Implementation ---

  void save_to_file(const std::string& path);

  void load_from_file(const std::string& path);
} // namespace musicdb