#include "ui/zincgui/widget.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/layout.hpp"
#include "ui/zincgui/root.hpp"

vec2i zincgui::Widget::get_position(Anchor relative_to) const {
  vec2i value = anchor_to_uv(parent_anchor) * vec2f{ui.get_window_size()};
  if (parent) { value = parent->get_position(parent_anchor); }
  value += vec2f(x, y) - vec2f(width, height) * anchor_to_uv(anchor);
  value += vec2f{width, height} * anchor_to_uv(relative_to);
  return value;
}

void zincgui::Widget::draw() {}

void zincgui::Widget::input() {}

void zincgui::Widget::update() {
#ifdef WIDGET_DRAW_DEBUG_RECT
  if (!is_debug_rect) {
    if (!debug_rect) {
      debug_rect = &add_child<Sprite>();
      debug_rect->is_debug_rect = true;
      debug_rect->ignore_parents_layout = true;
      static_cast<Sprite*>(debug_rect)->set_texture("red");
      static_cast<Sprite*>(debug_rect)->set_nine_slice_margin(1);
      static_cast<Sprite*>(debug_rect)->set_nine_slice_scale(1);
    }
    static_cast<Sprite*>(debug_rect)->set_size(width, height);
  }
#endif

  if (layout.enabled) { layout_engine::apply_layout(*this); }
}

bool zincgui::Widget::is_mouse_hovering() const { return is_mouse_hovering(Input::get_mouse_pos()); }

bool zincgui::Widget::is_mouse_hovering(vec2i at) const {
  const Widget* tested = this;
  i32 depth = hover_test_parent ? 127 : 1;
  for (i32 i = 0; i < depth; i += 1) {
    if (!tested) { break; }
    bool test_x =
      at.x >= tested->get_position(Anchor::TOP_LEFT).x && at.x < tested->get_position(Anchor::BOTTOM_RIGHT).x;
    bool test_y =
      at.y >= tested->get_position(Anchor::TOP_LEFT).y && at.y < tested->get_position(Anchor::BOTTOM_RIGHT).y;
    if (!test_x || !test_y) { return false; }
    tested = tested->parent;
  }
  return true;
}

void zincgui::Widget::set_layout(std::string_view def) {
  Layout l = layout_engine::parse_layout_string(def, layout);
  if (layout != l) {
    layout = l;
    mark_dirty();
  }
}

void zincgui::Widget::set_pos(i32 x_, i32 y_) {
  if (x_ != x || y_ != y) {
    x = x_;
    y = y_;
    mark_dirty();
  }
}

void zincgui::Widget::set_pos(vec2i pos) {
  if (pos.x != x || pos.y != y) {
    x = pos.x;
    y = pos.y;
    mark_dirty();
  }
}

void zincgui::Widget::set_size(i32 w_, i32 h_) {
  if (w_ != width || h_ != height) {
    width = w_;
    height = h_;
    mark_dirty();
  }
}

void zincgui::Widget::set_size(vec2i size_) {
  if (width != size_.x || height != size_.y) {
    width = size_.x;
    height = size_.y;
    mark_dirty();
  }
}

void zincgui::Widget::set_width(i32 width_) {
  if (width_ != width) {
    width = width_;
    mark_dirty();
  }
}

void zincgui::Widget::set_height(i32 height_) {
  if (height_ != height) {
    height = height_;
    mark_dirty();
  }
}

void zincgui::Widget::mark_dirty() {
  dirty = true;
  ui.mark_tree_dirty();
}

bool zincgui::Widget::is_focused() const { return ui.focused_widget() == this; }
bool zincgui::Widget::is_focus_manager() const { return ui.focus_manager() == this; }
