#include "sprite.hpp"
#include <cstddef>
#include <functional>
#include <glm/geometric.hpp>
#include "opengl_includes.hpp"
#include "ui.hpp"
#include "widget.hpp"

Sprite::Sprite(UI& ui_) : Widget(ui_) {}

Sprite::Sprite(UI& ui_, i32 width_, i32 height_) : Widget(ui_, width_, height_) {}

Sprite::~Sprite() {}

void Sprite::update() { Widget::update(); }

void Sprite::draw() {
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

void Sprite::update_mesh() {
  i32 window_width_ = ui.get_window_width();
  i32 window_height_ = ui.get_window_height();

  vec2f size_screen_uv = vec2f(width, height) / vec2f(window_width_, window_height_);

  vec2f start = vec2f(get_position()) / vec2f{window_width_, window_height_};
  vec2f end = start + size_screen_uv;

  // Translate range from [0, 1] to [-1, 1]
  start = start * vec2f(2.0) - vec2f(1.0);
  end = end * vec2f(2.0) - vec2f(1.0);

  vertices = {
    vertex_sprite{0,
                  {start.x, end.y},
                  {uv_start.x, uv_end.y},
                  nine_slice_margin,
                  nine_slice_scale,
                  {width, height},
                  uv_start,
                  uv_end},
    vertex_sprite{
      0, {end.x, end.y}, {uv_end.x, uv_end.y}, nine_slice_margin, nine_slice_scale, {width, height}, uv_start, uv_end},
    vertex_sprite{0,
                  {start.x, start.y},
                  {uv_start.x, uv_start.y},
                  nine_slice_margin,
                  nine_slice_scale,
                  {width, height},
                  uv_start,
                  uv_end},
    vertex_sprite{0,
                  {end.x, start.y},
                  {uv_end.x, uv_start.y},
                  nine_slice_margin,
                  nine_slice_scale,
                  {width, height},
                  uv_start,
                  uv_end},
  };
}

void Sprite::setup_buffers() {
  if (vbo != 0) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(vertex_sprite), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return;
  }

  glGenBuffers(1, &vbo);

  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_sprite), vertices.data(), GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 1, GL_INT, GL_FALSE, sizeof(vertex_sprite), (void*)offsetof(vertex_sprite, type));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite), (void*)offsetof(vertex_sprite, pos));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite), (void*)(offsetof(vertex_sprite, uv)));
  glEnableVertexAttribArray(2);

  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite),
                        (void*)(offsetof(vertex_sprite, nine_slice_margin)));
  glEnableVertexAttribArray(3);

  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite),
                        (void*)(offsetof(vertex_sprite, nine_slice_scale)));
  glEnableVertexAttribArray(4);

  glVertexAttribIPointer(5, 2, GL_UNSIGNED_INT, sizeof(vertex_sprite), (void*)(offsetof(vertex_sprite, widget_size)));
  glEnableVertexAttribArray(5);

  glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite), (void*)(offsetof(vertex_sprite, uv_start)));
  glEnableVertexAttribArray(6);

  glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_sprite), (void*)(offsetof(vertex_sprite, uv_end)));
  glEnableVertexAttribArray(7);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

TextureAtlas& Sprite::get_texture_atlas() { return ui.get_texture_atlas(); }

void Sprite::set_texture(std::string id, bool resize_to_texture_size) {
  auto val = get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warning("atlas texture not found: {}", id);
    return;
  }
  uv_start = val->get().start;
  uv_end = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (resize_to_texture_size) {
    width = texture_width;
    height = texture_height;
  }

  mark_dirty(); // FIXME check if texture actually changed
}
