#pragma once
#include "common/types.hpp"

namespace zincgui {
  enum class VertexType : i32 {
    SPRITE = 0,
    LABEL = 1,
    COLOR_RECT = 2,
  };

  struct Vertex {
      // common
      VertexType type{};
      vec2f pos;
      vec2f uv;

      // type = SPRITE
      float nine_slice_margin = 0.0f;
      float nine_slice_scale = 1.0f;
      glm::vec<2, u32> widget_size = {1.0f, 1.0f};
      vec2f uv_start = {0.0f, 0.0f};
      vec2f uv_end = {1.0f, 1.0f};

      // type = TEXT
      u32 atlas_i = 1;
      vec3f color{1.0};
      float opacity{1.0f};

      vec2f scissor_begin{};
      vec2f scissor_end{};
  };

} // namespace zincgui
