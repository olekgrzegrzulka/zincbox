#pragma once
#include <string>
#include <string_view>
#include "lib/utfcpp/source/utf8.h"

inline std::string utf32_to_utf8(std::u32string_view str) {
  std::string ret;
  try {
    utf8::utf32to8(str.begin(), str.end(), std::back_inserter(ret));
  } catch (const utf8::invalid_utf8& e) {
  }
  return ret;
};

inline std::u32string utf8_to_utf32(std::string_view str) {
  std::u32string ret;
  try {
    utf8::utf8to32(str.begin(), str.end(), std::back_inserter(ret));
  } catch (const utf8::invalid_utf8& e) {
  }
  return ret;
};
