#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include "common/logger.hpp"
#include "lib/utfcpp/source/utf8.h"

inline std::string utf32_to_utf8(std::u32string_view str) {
  std::string ret;
  try {
    utf8::utf32to8(str.begin(), str.end(), std::back_inserter(ret));
  } catch (const utf8::invalid_utf8& e) { out::debug_error("utf32 to utf conversion failed: {}", e.what()); }
  return ret;
};

inline std::u32string utf8_to_utf32(std::string_view str) {
  std::u32string ret;
  try {
    utf8::utf8to32(str.begin(), str.end(), std::back_inserter(ret));
  } catch (const utf8::invalid_utf8& e) { out::debug_error("utf32 to utf conversion failed: {}", e.what()); }
  return ret;
};

inline std::string path_to_utf8(const std::filesystem::path& p) {
  auto p_utf8 = p.u8string();
  return std::string(p_utf8.begin(), p_utf8.end());
}

inline std::filesystem::path utf8_to_path(std::string_view utf8_str) {
  return std::filesystem::path(
    std::u8string_view(reinterpret_cast<const char8_t*>(utf8_str.data()), utf8_str.size())
  );
}
