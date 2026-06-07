#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

template <size_t Size, class T> struct vertex {
  public:
    glm::vec<Size, T> pos;
    glm::vec<2, T> uv;

    vertex() = default;

    vertex(glm::vec<Size, T> pos_, glm::vec<2, T> uv_) {
      pos = pos_;
      uv = uv_;
    };
};

using vertex2 = vertex<2, float>;
using vertex3 = vertex<3, float>;