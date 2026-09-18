#include "tr.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include "common/utf.hpp"

namespace tr {
  static std::unordered_map<std::string, std::u32string> dictionary;

  bool load_from_string(const std::string& json_content) {
    dictionary.clear();

    glz::generic parsed_json{};
    const auto ec = glz::read_json(parsed_json, json_content);
    if (ec || !parsed_json.is_object()) { return false; }

    for (const auto& [key, value] : parsed_json.get_object()) {
      if (value.is_string()) { dictionary[key] = utf8_to_utf32(value.get_string()); }
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
