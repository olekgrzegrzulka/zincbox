#version 450 core

layout (binding = 0) uniform sampler2D atlas;

uniform vec2 tex_size;

layout (location = 0) in vec2 uv;
layout (location = 1) in float nine_slice_pixel_margin;
layout (location = 2) in float scale;
layout (location = 3) in flat uvec2 widget_size;
layout (location = 4) in vec2 uv_start;
layout (location = 5) in vec2 uv_end;

out vec4 FragColor;

void main() {
  vec2 uv_extents = uv_end - uv_start;
  vec2 tex_res = vec2(1.0) / tex_size;
  
  // uv2 is transposed to [0;1]
  vec2 uv2 = (uv - uv_start) / uv_extents;

  if (uv2.x <= nine_slice_pixel_margin / widget_size.x * scale) {
    uv2.x *= widget_size.x / tex_size.x / scale;
  } else if (uv2.x >= 1.0 - nine_slice_pixel_margin / widget_size.x * scale) {
    uv2.x -= 1.0;
    uv2.x *= widget_size.x / tex_size.x / scale;
    uv2.x += 1.0;
  } else {
    uv2.x -= (nine_slice_pixel_margin / widget_size.x);
    uv2.x /= (1.0 - nine_slice_pixel_margin / widget_size.x) - (nine_slice_pixel_margin / widget_size.x);
    uv2.x = mix(nine_slice_pixel_margin * tex_res.x, 1.0 - nine_slice_pixel_margin * tex_res.x, uv2.x);
  }

  if (uv2.y <= nine_slice_pixel_margin / widget_size.y * scale) {
    uv2.y *= widget_size.y / tex_size.y / scale;
  } else if (uv2.y >= 1.0 - nine_slice_pixel_margin / widget_size.y * scale) {
    uv2.y -= 1.0;
    uv2.y *= widget_size.y / tex_size.y / scale;
    uv2.y += 1.0;
  } else {
    uv2.y -= (nine_slice_pixel_margin / widget_size.y);
    uv2.y /= (1.0 - nine_slice_pixel_margin / widget_size.y) - (nine_slice_pixel_margin / widget_size.y);
    uv2.y = mix(nine_slice_pixel_margin * tex_res.y, 1.0 - nine_slice_pixel_margin * tex_res.y, uv2.y);
  }
  
  uv2 = uv2 * uv_extents + uv_start;
  vec4 color = texture(atlas, uv2);
  FragColor = color;
}