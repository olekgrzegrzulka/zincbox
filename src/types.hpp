#pragma once
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

enum Dir {
  NONE = 0,
  LEFT = 1,
  RIGHT = 2,
  FRONT = 4,
  BACK = 8,
  TOP = 16,
  BOTTOM = 32,
};

static constexpr Dir opposite_dir(Dir dir) {
  if (dir == Dir::LEFT) {
    return Dir::RIGHT;
  } else if (dir == Dir::RIGHT) {
    return Dir::LEFT;
  } else if (dir == Dir::FRONT) {
    return Dir::BACK;
  } else if (dir == Dir::BACK) {
    return Dir::FRONT;
  } else if (dir == Dir::TOP) {
    return Dir::BOTTOM;
  } else if (dir == Dir::BOTTOM) {
    return Dir::TOP;
  }
  return Dir::NONE;
}

struct rgba {
  u8 r{};
  u8 g{};
  u8 b{};
  u8 a{};

  auto operator<=>(const rgba& rhs) const = default;
};

struct hsva {
  double h{};
  double s{};
  double v{};
  double a{};

  auto operator<=>(const hsva& rhs) const = default;
};

using vec2i = glm::vec<2, i32>;
using vec2f = glm::vec<2, float>;
using vec2d = glm::vec<2, double>;
using vec3i = glm::vec<3, i32>;
using vec3f = glm::vec<3, float>;
using vec3d = glm::vec<3, double>;

struct rect2i {
  vec2i begin{0, 0};
  vec2i size{0, 0};

  [[nodiscard]] rect2i expanded(vec2i to) const {
    if (size == glm::vec<2, i32>{0, 0}) {
      return rect2i{
        .begin = to,
        .size = vec2i(1),
      };
    }

    vec2i current_min = begin;
    vec2i current_max = begin + size - vec2i(1);

    vec2i new_min = glm::min(current_min, to);
    vec2i new_max = glm::max(current_max, to);

    return rect2i{
      .begin = new_min,
      .size = new_max - new_min + vec2i(1),
    };
  }

  [[nodiscard]] rect2i expanded(const rect2i& to) const {
    if (to.size == vec2i{0, 0}) {
      return rect2i{
        .begin = begin,
        .size = size,
      };
    }
    return rect2i{.begin = begin, .size = size}
      .expanded(to.begin)
      .expanded(to.begin + to.size - vec2i{1, 1});
  }

  [[nodiscard]] rect2i intersected(const rect2i& b) const {
    vec2i min_a = begin;
    vec2i max_a = begin + size;

    vec2i min_b = b.begin;
    vec2i max_b = b.begin + b.size;

    vec2i intersect_min = glm::max(min_a, min_b);
    vec2i intersect_max = glm::min(max_a, max_b);

    vec2i intersect_size = intersect_max - intersect_min;

    if (intersect_size.x <= 0 || intersect_size.y <= 0) {
      return rect2i{.begin = {0, 0}, .size = {0, 0}};
    }

    return rect2i{
      .begin = intersect_min,
      .size = intersect_size};
  }

  bool is_empty() const {
    return size.x <= 0 || size.y <= 0;
  }
};

enum class BufferMode : i32 {
  BLEND,
  ERASE,
};

[[maybe_unused]] inline rgba hsva_to_rgba(double h, double s, double v, double a) {
  double r = 0, g = 0, b = 0;

  if (h >= 360.0) h = 0.0;
  if (h < 0.0) h = 0.0;

  int i = static_cast<int>(h / 60.0);
  double f = (h / 60.0) - i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);
  double t = v * (1.0 - s * (1.0 - f));

  switch (i) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  case 5:
  default:
    r = v;
    g = p;
    b = q;
    break;
  }

  return {
    static_cast<unsigned char>(r * 255.0 + 0.5),
    static_cast<unsigned char>(g * 255.0 + 0.5),
    static_cast<unsigned char>(b * 255.0 + 0.5),
    static_cast<unsigned char>(a * 255.0 + 0.5)};
}

[[maybe_unused]] inline hsva rgba_to_hsva(rgba color) {
  double r = color.r / 255.0;
  double g = color.g / 255.0;
  double b = color.b / 255.0;
  double a = color.a / 255.0;

  double max_val = std::max({r, g, b});
  double min_val = std::min({r, g, b});
  double delta = max_val - min_val;

  double h = 0.0;
  double s = 0.0;
  double v = max_val;

  if (delta > 0.0) {
    if (max_val == r) {
      h = 60.0 * (std::fmod(((g - b) / delta), 6.0));
    } else if (max_val == g) {
      h = 60.0 * (((b - r) / delta) + 2.0);
    } else if (max_val == b) {
      h = 60.0 * (((r - g) / delta) + 4.0);
    }

    if (h < 0.0) {
      h += 360.0;
    }
  } else {
    h = 0.0;
  }

  if (max_val > 0.0) {
    s = delta / max_val;
  } else {
    s = 0.0;
  }

  return {h, s, v, a};
}