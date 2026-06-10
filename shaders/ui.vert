#version 450 core

uniform mat4 matrix;

layout(location = 0) in int type;
layout(location = 1) in vec2 vertex;
layout(location = 2) in vec2 uv;
layout(location = 3) in float nine_slice_pixel_margin;
layout(location = 4) in float scale;
layout(location = 5) in uvec2 widget_size;
layout(location = 6) in vec2 uv_start;
layout(location = 7) in vec2 uv_end;
layout(location = 8) in vec3 text_color;

layout(location = 0) out int type_;
layout(location = 1) out vec2 uv_;
layout(location = 2) out float nine_slice_pixel_margin_;
layout(location = 3) out float scale_;
layout(location = 4) out uvec2 widget_size_;
layout(location = 5) out vec2 uv_start_;
layout(location = 6) out vec2 uv_end_;
layout(location = 7) out vec3 text_color_;

void main() {
  type_ = type;
  if (type == 0) {
    gl_Position = vec4(vertex, 0.0f, 1.0f) * matrix;
    uv_ = uv;
    nine_slice_pixel_margin_ = nine_slice_pixel_margin;
    scale_ = scale;
    widget_size_ = widget_size;
    uv_start_ = uv_start;
    uv_end_ = uv_end;
  } else {
    gl_Position = vec4(vertex, 0.0f, 1.0f) * matrix;
    uv_ = uv;
    text_color_ = text_color;
  }
}
