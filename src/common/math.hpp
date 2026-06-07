#pragma once
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

template <class T> static constexpr T manhattan_distance(glm::vec<2, T> first, glm::vec<2, T> second) {
  return std::abs(first.x - second.x) + std::abs(first.y - second.y);
}

template <class T> static constexpr T manhattan_distance(glm::vec<3, T> first, glm::vec<3, T> second) {
  return std::abs(first.x - second.x) + std::abs(first.y - second.y) + std::abs(first.z - second.z);
}

template <typename T>
static void sort_vector_by_manhattan_distance(std::vector<glm::vec<3, T>>& vector, glm::vec<3, T> to) {
  using Vec3T = glm::vec<3, T>;
  std::sort(vector.begin(), vector.end(), [&](const Vec3T a, const Vec3T b) {
    glm::vec<3, T> first = glm::abs(to - a);
    glm::vec<3, T> second = glm::abs(to - b);
    return first.x + first.y + first.z < second.x + second.y + second.z;
  });
}

// Adapted from
// https://github.com/godotengine/godot/blob/0eadbdb5d0709e4e557e52377fa075d3e2f0ad1f/core/math/math_funcs.h#L511
template <class T> constexpr T wrapi(T value, T min, T max) {
  T range = max - min;
  return range == 0 ? min : min + ((((value - min) % range) + range) % range);
}