#pragma once
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
