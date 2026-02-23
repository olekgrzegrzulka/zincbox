#include <algorithm>
#include <codecvt>
#include <limits>
#include <locale>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "../../lib/utfcpp/source/utf8.h"
#include "../opengl_includes.hpp"
#include "../shader.hpp"
#include "../types.hpp"
#include "font_face.hpp"
#include "label.hpp"
#include "sprite.hpp"
#include "ui.hpp"
#include "widget.hpp"

Label::Label(UI& ui_) : Widget::Widget(ui_) {
}

Label::Label(UI& ui_, std::string text_) : Widget::Widget(ui_) {
  set_text(text_);
  update_mesh();
}

Label::~Label() {
}

void Label::update() {
}

void Label::draw() {
  if (dirty) {
    update_mesh();
    setup_buffers();
    dirty = false;
  }

  auto& text_shader = ui.get_text_shader();
  auto& font_face = ui.get_font_face();

  text_shader.use();
  text_shader.set_uniform_mat4("matrix", ui.get_matrix());
  text_shader.set_uniform_float("color", text_color.r, text_color.g, text_color.b);
  font_face.bind(0);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Label::update_mesh() {
  i32 window_width_ = ui.get_window_width();
  i32 window_height_ = ui.get_window_height();

  vertices.clear();
  text_extents = {};

  float line_height = 0.0f;
  float current_line_width = 0.0f;
  const float line_spacing = 4.0f;

  // static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  // auto text_utf32 = converter.from_bytes(text);
  std::u32string text_utf32;
  try {
    utf8::utf8to32(text.begin(), text.end(), std::back_inserter(text_utf32));
  } catch (const utf8::invalid_utf8& e) {
  }

  for (auto c : text_utf32) {
    if (c == '\n') {
      text_extents.x = std::max(text_extents.x, current_line_width);
      text_extents.y += line_height + line_spacing;
      current_line_width = 0.0f;
      continue;
    }

    auto* glyph = ui.get_font_face().find_glyph(c);
    if (!glyph) continue;

    current_line_width += glyph->advance.x / 64.0f;
    line_height = std::max(line_height, (float)glyph->size.y);
  }

  text_extents.x = std::max(text_extents.x, current_line_width);
  text_extents.y += line_height;

  vec2f start_pos = get_position();
  start_pos += vec2i((anchor_to_uv(label_anchor)) * (vec2f(width, height) - text_extents));

  vec2f pen = start_pos;
  pen.y += line_height;

  for (auto c : text_utf32) {
    if (c == '\n') {
      pen.x = start_pos.x;
      pen.y += line_height + line_spacing;
      continue;
    }

    auto* glyph = ui.get_font_face().find_glyph(c);
    if (!glyph) continue;

    auto size_screen_uv = vec2f(glyph->size.x, glyph->size.y) / vec2f(window_width_, window_height_);

    auto position_screen_uv = vec2f(pen.x + glyph->bearing.x, pen.y - glyph->bearing.y) / vec2f{window_width_, window_height_};

    auto start = position_screen_uv;
    auto end = start + size_screen_uv;

    start = start * vec2f(2.0f) - vec2f(1.0f);
    end = end * vec2f(2.0f) - vec2f(1.0f);

    auto uv_start = glyph->uv_start;
    auto uv_end = glyph->uv_end;

    vertices.emplace_back(vertex2{{start.x, end.y}, {uv_start.x, uv_end.y}});
    vertices.emplace_back(vertex2{{end.x, end.y}, {uv_end.x, uv_end.y}});
    vertices.emplace_back(vertex2{{start.x, start.y}, {uv_start.x, uv_start.y}});

    vertices.emplace_back(vertex2{{end.x, end.y}, {uv_end.x, uv_end.y}});
    vertices.emplace_back(vertex2{{end.x, start.y}, {uv_end.x, uv_start.y}});
    vertices.emplace_back(vertex2{{start.x, start.y}, {uv_start.x, uv_start.y}});

    pen.x += glyph->advance.x / 64.0f;
  }
}

void Label::setup_buffers() {
  if (vbo == 0) { glGenBuffers(1, &vbo); }
  if (vao == 0) { glGenVertexArrays(1, &vao); }

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex2), vertices.data(), GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2), (void*)offsetof(vertex2, pos));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2), (void*)(offsetof(vertex2, uv)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}