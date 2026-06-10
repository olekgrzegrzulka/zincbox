#include <algorithm>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"
#include "opengl_includes.hpp"
#include "ui/theme.hpp" // FIXME ui_generic should not depend on ui/theme.hpp
#include "ui_generic/font_face.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/shader.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

Label::Label(UI& ui_) : Widget::Widget(ui_) { set_text_color(theme::get_prop("text_color").as_rgba()); }

Label::Label(UI& ui_, std::string_view text_) : Widget::Widget(ui_) {
  set_text_color(theme::get_prop("text_color").as_rgba());
  set_text(text_);
  update_mesh();
}

Label::Label(UI& ui_, std::u32string_view text_) : Widget::Widget(ui_) {
  set_text_color(theme::get_prop("text_color").as_rgba());
  set_text(text_);
  update_mesh();
}

Label::~Label() {
  if (vbo != 0) { glDeleteBuffers(1, &vbo); }
  if (vao != 0) { glDeleteVertexArrays(1, &vao); }
}

void Label::set_resize_to_text_extents(bool to) {
  if (to != resize_to_text_extents) { resize_to_text_extents = to; }
  if (resize_to_text_extents) {
    set_width(text_extents.x);
    set_height(text_extents.y);
    set_min_width(text_extents.x);
    set_min_height(text_extents.y);
  }
}

void Label::update() {
  if (dirty && is_drawn) {
    update_mesh();
    setup_buffers();
    dirty = false;
  }
  Widget::update();
}

void Label::draw() {
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());
  Widget::draw();
}

void Label::update_mesh() {
  i32 window_width_ = ui.get_window_width();
  i32 window_height_ = ui.get_window_height();

  vertices.clear();
  text_extents = {};

  float line_height = 0.0f;
  float current_line_width = 0.0f;
  const float line_spacing = 4.0f;

  // static std::u32string<std::codecvt_utf8<char32_t>, char32_t> converter;
  // auto text_utf32 = converter.from_bytes(text);

  for (auto c : text) {
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

  for (auto c : text) {
    if (c == '\n') {
      pen.x = start_pos.x;
      pen.y += line_height + line_spacing;
      continue;
    }

    auto* glyph = ui.get_font_face().find_glyph(c);
    if (!glyph) continue;

    auto size_screen_uv = vec2f(glyph->size.x, glyph->size.y) / vec2f(window_width_, window_height_);

    auto position_screen_uv =
      vec2f(pen.x + glyph->bearing.x, pen.y - glyph->bearing.y) / vec2f{window_width_, window_height_};

    auto start = position_screen_uv;
    auto end = start + size_screen_uv;

    start = start * vec2f(2.0f) - vec2f(1.0f);
    end = end * vec2f(2.0f) - vec2f(1.0f);

    auto uv_start = glyph->uv_start;
    auto uv_end = glyph->uv_end;

    vertices.emplace_back(vertex_label{.pos = {start.x, end.y}, .uv = {uv_start.x, uv_end.y}, .color = text_color});
    vertices.emplace_back(vertex_label{.pos = {end.x, end.y}, .uv = {uv_end.x, uv_end.y}, .color = text_color});
    vertices.emplace_back(vertex_label{.pos = {start.x, start.y}, .uv = {uv_start.x, uv_start.y}, .color = text_color});

    vertices.emplace_back(vertex_label{.pos = {end.x, end.y}, .uv = {uv_end.x, uv_end.y}, .color = text_color});
    vertices.emplace_back(vertex_label{.pos = {end.x, start.y}, .uv = {uv_end.x, uv_start.y}, .color = text_color});
    vertices.emplace_back(vertex_label{.pos = {start.x, start.y}, .uv = {uv_start.x, uv_start.y}, .color = text_color});

    pen.x += glyph->advance.x / 64.0f;
  }

  if (resize_to_text_extents) {
    set_width(text_extents.x);
    set_height(text_extents.y);
    set_min_width(text_extents.x);
    set_min_height(text_extents.y);
  }
}

void Label::setup_buffers() {
  if (vbo == 0) { glGenBuffers(1, &vbo); }
  if (vao == 0) { glGenVertexArrays(1, &vao); }

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_label), vertices.data(), GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 1, GL_INT, GL_FALSE, sizeof(vertex_label), (void*)offsetof(vertex_label, type));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_label), (void*)offsetof(vertex_label, pos));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_label), (void*)(offsetof(vertex_label, uv)));
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_label), (void*)(offsetof(vertex_label, color)));
  glEnableVertexAttribArray(8);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
