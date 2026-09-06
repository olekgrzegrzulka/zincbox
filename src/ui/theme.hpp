#pragma once
#include <set>
#include <string>
#include <string_view>
#include "theme_config.hpp"

class UI;

namespace theme {
  std::set<std::string> get_themes();
  std::set<std::string> get_languages();
  void load_default_theme(UI&, std::string_view language = "en-US");
  void load_theme(std::string_view name, UI&, std::string_view language = "en-US");
  const std::vector<uint8_t>& get_raw_resource(const std::string& path);
  const ThemeConfig& config();
} // namespace theme
