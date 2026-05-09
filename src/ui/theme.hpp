#pragma once
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include "common/color.hpp"
#include "common/types.hpp"

class UI;

namespace theme {
  struct theme_prop final {
      std::variant<i32, rgba, std::string, double, std::monostate> value;

      i32 as_i32(i32 default_value = 0) const {
        if (std::holds_alternative<i32>(value)) { return std::get<i32>(value); }
        return default_value;
      }

      rgba as_rgba(rgba default_value = {}) const {
        if (std::holds_alternative<rgba>(value)) { return std::get<rgba>(value); }
        return default_value;
      }

      std::string as_string(std::string default_value = {}) const {
        if (std::holds_alternative<std::string>(value)) { return std::get<std::string>(value); }
        return default_value;
      }

      double as_double(double default_value = 0.0) const {
        if (std::holds_alternative<double>(value)) { return std::get<double>(value); }
        return default_value;
      }
  };

  static constexpr i32 playlist_cover_width = 64 + 12;
  static constexpr i32 playlist_cover_height = 64 + 32;

  theme_prop get_prop(std::string_view prop);
  i32 get_button_nine_slice_margin(std::string_view name);
  std::set<std::string> get_themes();
  void load_default_theme(UI&);
  void load_theme(std::string_view name, UI&);
} // namespace theme
