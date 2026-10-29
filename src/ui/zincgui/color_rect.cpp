#include "ui/zincgui/color_rect.hpp"
#include <glad/glad.h>
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

zincgui::ColorRect::ColorRect(Root& ui_) : Widget(ui_) {}

zincgui::ColorRect::ColorRect(Root& ui_, i32 width_, i32 height_) : Widget(ui_, width_, height_) {}

zincgui::ColorRect::ColorRect(Root& ui_, rgba color_) : Widget(ui_) { color(color_); }

zincgui::ColorRect::~ColorRect() = default;

void zincgui::ColorRect::update() { Widget::update(); }

std::span<const zincgui::Vertex> zincgui::ColorRect::vertices() {
  i32 window_width_ = ui().get_window_width();
  i32 window_height_ = ui().get_window_height();

  vec2f size_screen_uv = vec2f(width(), height()) / vec2f(window_width_, window_height_);

  vec2f start = vec2f(position()) / vec2f{window_width_, window_height_};
  vec2f end = start + size_screen_uv;

  // Translate range from [0, 1] to [-1, 1]
  start = start * vec2f(2.0) - vec2f(1.0);
  end = end * vec2f(2.0) - vec2f(1.0);

  m_vertices = {
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{start.x, end.y}, .color = m_color, .opacity = m_opacity},
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{end.x, end.y}, .color = m_color, .opacity = m_opacity},
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{start.x, start.y}, .color = m_color, .opacity = m_opacity},
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{start.x, start.y}, .color = m_color, .opacity = m_opacity},
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{end.x, end.y}, .color = m_color, .opacity = m_opacity},
    Vertex{.type = VertexType::COLOR_RECT, .pos = vec2f{end.x, start.y}, .color = m_color, .opacity = m_opacity},
  };

  return m_vertices;
}
