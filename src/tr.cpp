#include "tr.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <fmt/format.h>
#include "common/utf.hpp"
#include "lib/json.cpp/json.h"

namespace tr {
  static std::unordered_map<std::string, std::u32string> dictionary;

  bool load_from_string(const std::string& json_content) {
    dictionary.clear();

    auto [status, parsed_json] = jt::Json::parse(json_content);
    if (status != jt::Json::success || !parsed_json.isObject()) { return false; }

    for (const auto& [key, value] : parsed_json.getObject()) {
      if (value.isString()) { dictionary[key] = utf8_to_utf32(value.getString()); }
    }

    return true;
  }

  bool load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) { return false; }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return load_from_string(buffer.str());
  }

  std::u32string get(const std::string& key) {
    auto it = dictionary.find(key);
    if (it != dictionary.end()) { return it->second; }
    return utf8_to_utf32(key);
  }
} // namespace tr
