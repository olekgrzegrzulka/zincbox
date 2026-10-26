#include "ui/zincgui/sprite.hpp"
#include <cstddef>
#include <functional>
#include <glad/glad.h>
#include "common/logger.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"
zincgui::Sprite::Sprite(Root& ui_) : Widget(ui_) {}

zincgui::Sprite::Sprite(Root& ui_, i32 width_, i32 height_) : Widget(ui_, width_, height_) {}

zincgui::Sprite::~Sprite() {}

std::vector<zincgui::Vertex> zincgui::Sprite::get_vertices() {
  i32 window_width_ = ui.get_window_width();
  i32 window_height_ = ui.get_window_height();

  vec2f size_screen_uv = vec2f(width, height) / vec2f(window_width_, window_height_);

  vec2f start = vec2f(get_position()) / vec2f{window_width_, window_height_};
  vec2f end = start + size_screen_uv;

  // Translate range from [0, 1] to [-1, 1]
  start = start * vec2f(2.0) - vec2f(1.0);
  end = end * vec2f(2.0) - vec2f(1.0);

  return {
    Vertex{VertexType::SPRITE,
           {start.x, end.y},
           {uv_start.x, uv_end.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, end.y},
           {uv_end.x, uv_end.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
    Vertex{VertexType::SPRITE,
           {start.x, start.y},
           {uv_start.x, uv_start.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
    Vertex{VertexType::SPRITE,
           {start.x, start.y},
           {uv_start.x, uv_start.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, end.y},
           {uv_end.x, uv_end.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, start.y},
           {uv_end.x, uv_start.y},
           nine_slice_margin,
           nine_slice_scale,
           {width, height},
           uv_start,
           uv_end,
           atlas_index},
  };
}

void zincgui::Sprite::update() { Widget::update(); }

zincgui::TextureAtlas& zincgui::Sprite::get_texture_atlas() { return ui.get_texture_atlas(); }

void zincgui::Sprite::set_texture(const std::string& id, bool resize_to_texture_size) {
  auto val = get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }

  set_uv_start(val->get().start);
  set_uv_end(val->get().end);

  if (texture_width != val->get().width) {
    texture_width = val->get().width;
    mark_dirty();
  }

  if (texture_height != val->get().height) {
    texture_height = val->get().height;
    mark_dirty();
  }

  if (resize_to_texture_size) {
    set_width(texture_width);
    set_height(texture_height);
  }
}
