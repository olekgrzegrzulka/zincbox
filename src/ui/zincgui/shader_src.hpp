namespace zincgui {

#pragma once

  static constexpr char shader_vert[] = R"(#version 450 core

uniform mat4 matrix;

layout(location = 0) in int type;
layout(location = 1) in vec2 vertex;
layout(location = 2) in vec2 uv;
layout(location = 3) in float nine_slice_pixel_margin;
layout(location = 4) in float scale;
layout(location = 5) in uvec2 widget_size;
layout(location = 6) in vec2 uv_start;
layout(location = 7) in vec2 uv_end;
layout(location = 8) in vec3 color;
layout(location = 9) in float opacity;

layout(location = 0) out flat int type_;
layout(location = 1) out vec2 uv_;
layout(location = 2) out float nine_slice_pixel_margin_;
layout(location = 3) out float scale_;
layout(location = 4) out flat uvec2 widget_size_;
layout(location = 5) out vec2 uv_start_;
layout(location = 6) out vec2 uv_end_;
layout(location = 7) out vec3 color_;
layout(location = 8) out float opacity_;

void main() {
  gl_Position = vec4(vertex, 0.0f, 1.0f) * matrix;
  type_ = type;
  if (type == 0) {
    uv_ = uv;
    nine_slice_pixel_margin_ = nine_slice_pixel_margin;
    scale_ = scale;
    widget_size_ = widget_size;
    uv_start_ = uv_start;
    uv_end_ = uv_end;
  } else if (type == 1) {
    uv_ = uv;
    color_ = color;
  } else if (type == 2) {
    color_ = color;
    opacity_ = opacity;
  }
})";

  static constexpr char shader_frag[] = R"(#version 450 core

layout(binding = 0) uniform sampler2D atlas;
layout(binding = 1) uniform sampler2D text_atlas;

uniform vec2 tex_size;

layout(location = 0) in flat int type;
layout(location = 1) in vec2 uv;
layout(location = 2) in float nine_slice_pixel_margin;
layout(location = 3) in float scale;
layout(location = 4) in flat uvec2 widget_size;
layout(location = 5) in vec2 uv_start;
layout(location = 6) in vec2 uv_end;
layout(location = 7) in vec3 color;
layout(location = 8) in float opacity;

out vec4 FragColor;

void main() {
  if (type == 0) {
    vec2 uv_extents = uv_end - uv_start;
    vec2 sprite_size = uv_extents * tex_size;
    
    vec2 uv2 = (uv - uv_start) / uv_extents;
    
    vec2 border_widget = min((nine_slice_pixel_margin * scale) / vec2(widget_size), vec2(0.4999));
    vec2 border_sprite = nine_slice_pixel_margin / sprite_size;
    
    if (uv2.x < border_widget.x) {
        uv2.x = (uv2.x / border_widget.x) * border_sprite.x;
    } else if (uv2.x > 1.0 - border_widget.x) {
        uv2.x = 1.0 - border_sprite.x + ((uv2.x - (1.0 - border_widget.x)) / border_widget.x) * border_sprite.x;
    } else {
        float t = (uv2.x - border_widget.x) / (1.0 - 2.0 * border_widget.x);
        uv2.x = mix(border_sprite.x, 1.0 - border_sprite.x, t);
    }
    
    if (uv2.y < border_widget.y) {
        uv2.y = (uv2.y / border_widget.y) * border_sprite.y;
    } else if (uv2.y > 1.0 - border_widget.y) {
        uv2.y = 1.0 - border_sprite.y + ((uv2.y - (1.0 - border_widget.y)) / border_widget.y) * border_sprite.y;
    } else {
        float t = (uv2.y - border_widget.y) / (1.0 - 2.0 * border_widget.y);
        uv2.y = mix(border_sprite.y, 1.0 - border_sprite.y, t);
    }
    
    FragColor = texture(atlas, uv_start + uv2 * uv_extents);
  } else if (type == 1) {
    float a = texture(text_atlas, uv).a;
    FragColor = vec4(color, a);
  } else if (type == 2) {
    FragColor = vec4(color, opacity);
  }
})";

} // namespace zincgui
