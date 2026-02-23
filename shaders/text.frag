#version 450 core

layout (binding = 0) uniform sampler2D atlas;
uniform vec3 color;

layout (location = 0) in vec2 uv;

out vec4 FragColor;

void main() {
  float a = texture(atlas, uv).a;
  FragColor = vec4(color, a);
}
