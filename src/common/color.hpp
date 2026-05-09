#pragma once
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "types.hpp"

struct hsva {
    double h{};
    double s{};
    double v{};
    double a{};

    auto operator<=>(const hsva& rhs) const = default;
};

struct rgba {
    u8 r{};
    u8 g{};
    u8 b{};
    u8 a{};

    auto operator<=>(const rgba& rhs) const = default;
};

namespace color_utils {
  static inline float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
  static inline u8 to_u8(float v) { return static_cast<u8>(std::round(clamp01(v) * 255.0f)); }

  static inline rgba hsl_to_rgb(float h, float s, float l, float a) {
    auto f = [&](float n) {
      float k = std::fmod(n + h / 30.0f, 12.0f);
      float a_coeff = s * std::min(l, 1.0f - l);
      return l - a_coeff * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
    };
    return {to_u8(f(0)), to_u8(f(8)), to_u8(f(4)), to_u8(a)};
  }

  static inline rgba hsv_to_rgb(float h, float s, float v, float a) {
    auto f = [&](float n) {
      float k = std::fmod(n + h / 60.0f, 6.0f);
      return v - v * s * std::max(0.0f, std::min({k, 4.0f - k, 1.0f}));
    };
    return {to_u8(f(5)), to_u8(f(3)), to_u8(f(1)), to_u8(a)};
  }

  static inline std::vector<float> parse_numbers(std::string_view s) {
    std::vector<float> nums;
    size_t start = s.find('(');
    size_t end = s.find(')');
    if (start == std::string::npos || end == std::string::npos) return {};

    std::string content(s.substr(start + 1, end - start - 1));
    char* p = content.data();
    char* end_ptr = p + content.size();
    while (p < end_ptr) {
      while (p < end_ptr && (std::isspace(*p) || *p == ','))
        p++;
      if (p >= end_ptr) break;
      char* next;
      float val = std::strtof(p, &next);
      if (p == next) break;
      nums.push_back(val);
      p = next;
    }
    return nums;
  }

  static inline std::optional<rgba> parse_color(std::string_view str) {
    if (str.empty()) return std::nullopt;

    std::string_view hex = str;
    if (hex[0] == '#') hex.remove_prefix(1);

    if (hex.length() == 6 || hex.length() == 8) {
      uint32_t val = 0;
      auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.length(), val, 16);
      if (ec == std::errc{}) {
        if (hex.length() == 6)
          return rgba{u8(val >> 16), u8(val >> 8), u8(val), 255};
        return rgba{u8(val >> 24), u8(val >> 16), u8(val >> 8), u8(val)};
      }
    }

    auto nums = color_utils::parse_numbers(str);
    bool is_float = str.find('.') != std::string_view::npos;

    if (str.starts_with("rgb")) {
      if (nums.size() < 3) return std::nullopt;
      float alpha = (nums.size() == 4) ? (is_float ? nums[3] : nums[3] / 255.0f) : 1.0f;
      if (is_float && nums[0] <= 1.0f && nums[1] <= 1.0f && nums[2] <= 1.0f)
        return rgba{color_utils::to_u8(nums[0]), color_utils::to_u8(nums[1]), color_utils::to_u8(nums[2]), color_utils::to_u8(alpha)};
      return rgba{u8(nums[0]), u8(nums[1]), u8(nums[2]), color_utils::to_u8(alpha)};
    }

    if (str.starts_with("hsl")) {
      if (nums.size() < 3) return std::nullopt;
      float h = nums[0], s = nums[1], l = nums[2];
      if (s > 1.0f) s /= 100.0f;
      if (l > 1.0f) l /= 100.0f;
      float a = (nums.size() == 4) ? (nums[3] > 1.0f ? nums[3] / 255.0f : nums[3]) : 1.0f;
      return color_utils::hsl_to_rgb(h, s, l, a);
    }

    if (str.starts_with("hsv")) {
      if (nums.size() < 3) return std::nullopt;
      float h = nums[0], s = nums[1], v = nums[2];
      if (s > 1.0f) s /= 100.0f;
      if (v > 1.0f) v /= 100.0f;
      float a = (nums.size() == 4) ? (nums[3] > 1.0f ? nums[3] / 255.0f : nums[3]) : 1.0f;
      return color_utils::hsv_to_rgb(h, s, v, a);
    }

    if (str == "transparent") return rgba{0, 0, 0, 0};
    if (str == "white") return rgba{255, 255, 255, 255};
    if (str == "black") return rgba{0, 0, 0, 255};

    return std::nullopt;
  }
} // namespace color_utils
