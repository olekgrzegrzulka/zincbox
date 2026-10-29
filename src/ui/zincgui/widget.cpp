#include "ui/zincgui/widget.hpp"
#include "ui/zincgui/layout.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"

using namespace zincgui;

zincgui::Widget::Widget(zincgui::Root& ui_) : m_ui{ui_} { mark_dirty(); }

zincgui::Widget::Widget(zincgui::Root& ui_, i32 width_, i32 height_) : m_ui{ui_}, m_width{width_}, m_height{height_} {
  mark_dirty();
}

zincgui::Widget::~Widget() = default;

void zincgui::Widget::input() {}

void zincgui::Widget::update() {
#ifdef WIDGET_DRAW_DEBUG_RECT
  if (!m_is_debug_rect) {
    if (!m_debug_rect) {
      m_debug_rect = &add_child<Sprite>();
      m_debug_rect->m_is_debug_rect = true;
      m_debug_rect->ignore_parents_layout(true);
      static_cast<Sprite*>(m_debug_rect)->texture(default_settings::debug_rect_texture)
                                         .nine_slice_margin(default_settings::debug_rect_nine_slice_margin)
                                         .nine_slice_scale(default_settings::debug_rect_nine_slice_scale);
    }
    static_cast<Sprite*>(m_debug_rect)->size(m_width, m_height);
  }
#endif
}

std::span<const Vertex> zincgui::Widget::vertices() {
  static const std::vector<Vertex> EMPTY = {};
  return EMPTY;
}

bool zincgui::Widget::is_mouse_hovering() const { return m_hovered; }

bool zincgui::Widget::is_mouse_hovering(vec2i at) const {
  const Widget* tested = this;
  i32 depth = m_hover_test_parent ? default_settings::widget_hover_max_depth : 1;
  for (i32 i = 0; i < depth; i += 1) {
    if (!tested) { break; }
    bool test_x =
      at.x >= tested->position(Anchor::TOP_LEFT).x && at.x < tested->position(Anchor::BOTTOM_RIGHT).x;
    bool test_y =
      at.y >= tested->position(Anchor::TOP_LEFT).y && at.y < tested->position(Anchor::BOTTOM_RIGHT).y;
    if (!test_x || !test_y) { return false; }
    tested = tested->m_parent;
  }
  return true;
}

bool zincgui::Widget::is_focused() const { return m_ui.focused_widget() == this; }

bool zincgui::Widget::is_focus_manager() const { return m_ui.focus_manager() == this; }

void zincgui::Widget::mark_layout_dirty() {
  m_layout_dirty = true;
  m_ui.mark_layout_dirty();
}

void zincgui::Widget::mark_dirty() {
  if (m_dirty) { return; }
  m_dirty = true;
  m_ui.mark_tree_dirty();
}

void zincgui::Widget::set_layout_impl(std::string_view def) {
  Layout l = layout_engine::parse_layout_string(def, m_layout);
  if (m_layout != l) {
    m_layout = l;
    mark_layout_dirty();
    mark_dirty();
  }
}
