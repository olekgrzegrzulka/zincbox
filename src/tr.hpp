#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include "common/logger.hpp"
#include "common/utf.hpp"

namespace tr {
  bool load_from_file(const std::filesystem::path&);
  bool load_from_string(const std::string& json_content);
  std::string get(const std::string& key);

  std::string_view get_fmt_string(const std::string& key);

  template <typename... Args> std::string format(const std::string& key, Args&&... args) {
    std::string fmt_str = get(key);
    try {
      return fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);
    } catch (const fmt::format_error& e) {
      out::error("tr::format: error key '{}': {}", key, e.what());
      return fmt_str;
    }
  }
} // namespace tr
