#include "color_rect.hpp"
#include <cstddef>
#include <glm/geometric.hpp>
#include "opengl_includes.hpp"
#include "ui.hpp"
#include "widget.hpp"

ColorRect::ColorRect(UI& ui_) : Widget(ui_) {}

ColorRect::ColorRect(UI& ui_, i32 width_, i32 height_) : Widget(ui_, width_, height_) {}

ColorRect::~ColorRect() {
  if (vbo != 0) { glDeleteBuffers(1, &vbo); }
  if (vao != 0) { glDeleteVertexArrays(1, &vao); }
}

void ColorRect::update() { Widget::update(); }

void ColorRect::draw() {
  if (dirty) {
    // FIXME: this can cause a 1 frame delay when children are updated BEFORE parent
    for (auto&& c : children) {
      c->mark_dirty();
    }
    update_mesh();
    setup_buffers();
    dirty = false;
  }

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  Widget::draw();
}

void ColorRect::update_mesh() {
  i32 window_width_ = ui.get_window_width();
  i32 window_height_ = ui.get_window_height();

  vec2f size_screen_uv = vec2f(width, height) / vec2f(window_width_, window_height_);

  vec2f start = vec2f(get_position()) / vec2f{window_width_, window_height_};
  vec2f end = start + size_screen_uv;

  // Translate range from [0, 1] to [-1, 1]
  start = start * vec2f(2.0) - vec2f(1.0);
  end = end * vec2f(2.0) - vec2f(1.0);

  vertices = {
    ColorRect::vertex(vec2f{start.x, end.y}, color),
    ColorRect::vertex(vec2f{end.x, end.y}, color),
    ColorRect::vertex(vec2f{start.x, start.y}, color),
    ColorRect::vertex(vec2f{end.x, start.y}, color),
  };
}

void ColorRect::setup_buffers() {
  if (vbo != 0) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(ColorRect::vertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return;
  }

  glGenBuffers(1, &vbo);
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ColorRect::vertex), vertices.data(), GL_DYNAMIC_DRAW);

  glVertexAttribIPointer(0, 1, GL_INT, sizeof(ColorRect::vertex), (void*)offsetof(ColorRect::vertex, type));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ColorRect::vertex), (void*)offsetof(ColorRect::vertex, pos));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(ColorRect::vertex),
                        (void*)(offsetof(ColorRect::vertex, color)));
  glEnableVertexAttribArray(8);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
