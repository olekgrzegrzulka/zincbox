#version 450 core

uniform  mat4 matrix;

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 uv_;

void main() {
  gl_Position = vec4(vertex, 0.0f, 1.0f) * matrix;
  uv_ = uv;
}
