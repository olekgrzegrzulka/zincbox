#include "ui/zincgui/sprite.hpp"
#include <functional>
#include <span>
#include <glad/glad.h>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

zincgui::Sprite::Sprite(Root& ui_) : Widget(ui_) {}

zincgui::Sprite::Sprite(Root& ui_, const std::string& id_) : Widget(ui_) {
  texture(id_, true);
}

zincgui::Sprite::Sprite(Root& ui_, i32 width_, i32 height_) : Widget(ui_, width_, height_) {}

zincgui::Sprite::~Sprite() = default;

void zincgui::Sprite::update() { Widget::update(); }

std::span<const zincgui::Vertex> zincgui::Sprite::vertices() {
  i32 window_width_ = ui().get_window_width();
  i32 window_height_ = ui().get_window_height();

  vec2f size_screen_uv = vec2f(width(), height()) / vec2f(window_width_, window_height_);

  vec2f start = vec2f(position()) / vec2f{window_width_, window_height_};
  vec2f end = start + size_screen_uv;

  // Translate range from [0, 1] to [-1, 1]
  start = start * vec2f(2.0) - vec2f(1.0);
  end = end * vec2f(2.0) - vec2f(1.0);

  m_vertices = {
    Vertex{VertexType::SPRITE,
           {start.x, end.y},
           {m_uv_start.x, m_uv_end.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, end.y},
           {m_uv_end.x, m_uv_end.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
    Vertex{VertexType::SPRITE,
           {start.x, start.y},
           {m_uv_start.x, m_uv_start.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
    Vertex{VertexType::SPRITE,
           {start.x, start.y},
           {m_uv_start.x, m_uv_start.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, end.y},
           {m_uv_end.x, m_uv_end.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
    Vertex{VertexType::SPRITE,
           {end.x, start.y},
           {m_uv_end.x, m_uv_start.y},
           m_nine_slice_margin,
           m_nine_slice_scale,
           {width(), height()},
           m_uv_start,
           m_uv_end,
           m_atlas_index},
  };

  return m_vertices;
}

zincgui::TextureAtlas& zincgui::Sprite::texture_atlas() { return ui().get_texture_atlas(); }

void zincgui::Sprite::set_texture_impl(const std::string& id, bool resize_to_texture_size) {
  auto val = texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }

  uv_start(val->get().start);
  uv_end(val->get().end);

  if (m_texture_width != val->get().width) {
    m_texture_width = val->get().width;
    mark_dirty();
  }

  if (m_texture_height != val->get().height) {
    m_texture_height = val->get().height;
    mark_dirty();
  }

  if (resize_to_texture_size) {
    width(m_texture_width);
    height(m_texture_height);
  }
}
