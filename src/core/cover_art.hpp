#pragma once
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include "core/musicdb/types.hpp"

namespace zincbox::cover_art {
  void update();
  void deserialize(std::ifstream&);
  void serialize(std::ofstream&, std::span<const size_t>);
  std::optional<std::filesystem::path> get_full_res(db::track_id_t);
  std::optional<std::filesystem::path> get_256(db::track_id_t);
  std::optional<std::filesystem::path> get_64(db::track_id_t);
} // namespace zincbox::cover_art
