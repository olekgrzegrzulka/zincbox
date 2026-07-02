#pragma once
#include <string>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include "common/utf.hpp"

namespace tr {
  bool load_from_file(const std::string& filepath);
  bool load_from_string(const std::string& json_content);
  std::u32string get(const std::string& key);

  std::u32string_view get_fmt_string(const std::string& key);

  template <typename... Args> std::u32string format(const std::string& key, Args&&... args) {
    std::u32string fmt_str = get(key);
    return utf8_to_utf32(fmt::format(fmt::runtime(utf32_to_utf8(fmt_str)), std::forward<Args>(args)...));
  }
} // namespace tr
