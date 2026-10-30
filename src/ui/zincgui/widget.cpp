#include "ui/zincgui/widget.hpp"
#include "ui/zincgui/layout.hpp"
#include "ui/zincgui/root.hpp"

using namespace zincgui;

zincgui::Widget::Widget(zincgui::Root& ui_) : ui{ui_} { mark_dirty(); }

zincgui::Widget::Widget(zincgui::Root& ui_, i32 width_, i32 height_) : ui{ui_}, width{width_}, height{height_} {
  mark_dirty();
}

vec2i zincgui::Widget::get_position(Anchor relative_to) const {
  return global_position_ + vec2i(vec2f{width, height} * anchor_to_uv(relative_to));
}

void zincgui::Widget::input() {}

void zincgui::Widget::update() {
#ifdef WIDGET_DRAW_DEBUG_RECT
  if (!is_debug_rect) {
    if (!debug_rect) {
      debug_rect = &add_child<Sprite>();
      debug_rect->is_debug_rect = true;
      debug_rect->ignore_parents_layout = true;
      static_cast<Sprite*>(debug_rect)->set_texture(default_settings::debug_rect_texture);
      static_cast<Sprite*>(debug_rect)->set_nine_slice_margin(default_settings::debug_rect_nine_slice_margin);
      static_cast<Sprite*>(debug_rect)->set_nine_slice_scale(default_settings::debug_rect_nine_slice_scale);
    }
    static_cast<Sprite*>(debug_rect)->set_size(width, height);
  }
#endif
}

bool zincgui::Widget::is_mouse_hovering() const { return hovered_; }

bool zincgui::Widget::is_mouse_hovering(vec2i at) const {
  const Widget* tested = this;
  i32 depth = hover_test_parent ? default_settings::widget_hover_max_depth : 1;
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
    layout_dirty = true;
    mark_dirty();
  }
}

void zincgui::Widget::set_pos(i32 x_, i32 y_) {
  set_x(x_);
  set_y(y_);
}

void zincgui::Widget::set_pos(vec2i pos) {
  set_x(pos.x);
  set_y(pos.y);
}

void zincgui::Widget::set_size(i32 w_, i32 h_) {
  set_width(w_);
  set_height(h_);
}

void zincgui::Widget::set_size(vec2i size_) {
  set_width(size_.x);
  set_height(size_.y);
}

namespace {
  constexpr bool is_horizontal(zincgui::LayoutDirection dir) noexcept {
    return dir == zincgui::LayoutDirection::LEFT_TO_RIGHT || dir == zincgui::LayoutDirection::RIGHT_TO_LEFT;
  }

  constexpr bool is_vertical(zincgui::LayoutDirection dir) noexcept {
    return dir == zincgui::LayoutDirection::TOP_TO_BOTTOM || dir == zincgui::LayoutDirection::BOTTOM_TO_TOP;
  }
} // namespace

void zincgui::Widget::set_x(i32 x_, bool ignore_constraints) {
  if (x == x_) { return; }
  if (!ignore_constraints && !ignore_parents_layout && parent && parent->layout.enabled) { return; }
  x = x_;
  mark_dirty();
}

void zincgui::Widget::set_y(i32 y_, bool ignore_constraints) {
  if (y == y_) { return; }
  if (!ignore_constraints && !ignore_parents_layout && parent && parent->layout.enabled) { return; }
  y = y_;
  mark_dirty();
}

void zincgui::Widget::set_width(i32 width_, bool ignore_constraints) {
  if (width == width_) { return; }
  if (!ignore_constraints) {
    if (layout.enabled && layout.fit_to_contents && is_horizontal(layout.direction)) { return; }
    if (!ignore_parents_layout && parent && parent->layout.enabled) {
      if (parent->layout.expand_children && is_vertical(parent->layout.direction)) { return; }
      if (parent->layout.fill && weight > 0.0f && is_horizontal(parent->layout.direction)) { return; }
    }
  }
  width = width_;
  mark_dirty();
  if (layout.enabled && (layout.expand_children || layout.fill)) { layout_dirty = true; }
}

void zincgui::Widget::set_height(i32 height_, bool ignore_constraints) {
  if (height == height_) { return; }
  if (!ignore_constraints) {
    if (layout.enabled && layout.fit_to_contents && is_vertical(layout.direction)) { return; }
    if (!ignore_parents_layout && parent && parent->layout.enabled) {
      if (parent->layout.expand_children && is_horizontal(parent->layout.direction)) { return; }
      if (parent->layout.fill && weight > 0.0f && is_vertical(parent->layout.direction)) { return; }
    }
  }
  height = height_;
  mark_dirty();
  if (layout.enabled && (layout.expand_children || layout.fill)) { layout_dirty = true; }
}

void zincgui::Widget::set_anchor(Anchor anchor_, bool ignore_constraints) {
  if (anchor == anchor_) { return; }
  if (!ignore_constraints && !ignore_parents_layout && parent && parent->layout.enabled) { return; }
  anchor = anchor_;
  mark_dirty();
}

void zincgui::Widget::set_parent_anchor(Anchor parent_anchor_, bool ignore_constraints) {
  if (parent_anchor == parent_anchor_) { return; }
  if (!ignore_constraints && !ignore_parents_layout && parent && parent->layout.enabled) { return; }
  parent_anchor = parent_anchor_;
  mark_dirty();
}

void zincgui::Widget::set_min_width(i32 v) {
  if (min_width == v) { return; }
  min_width = v;
  mark_dirty();
  if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
}

void zincgui::Widget::set_max_width(i32 v) {
  if (max_width == v) { return; }
  max_width = v;
  mark_dirty();
  if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
}

void zincgui::Widget::set_min_height(i32 v) {
  if (min_height == v) { return; }
  min_height = v;
  mark_dirty();
  if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
}

void zincgui::Widget::set_max_height(i32 v) {
  if (max_height == v) { return; }
  max_height = v;
  mark_dirty();
  if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
}

void zincgui::Widget::mark_dirty() {
  if (dirty) { return; }
  dirty = true;
  ui.mark_tree_dirty();
}

void Widget::set_update_children_first(bool value) {
  if (update_children_first != value) {
    update_children_first = value;
    mark_dirty();
  }
}
void Widget::set_is_self_drawn(bool value) {
  if (is_self_drawn != value) {
    is_self_drawn = value;
    mark_dirty();
  }
}
void Widget::set_clip(bool value) {
  if (clip != value) {
    clip = value;
    mark_dirty();
  }
}
void Widget::set_clip_children(bool value) {
  if (clip_children != value) {
    clip_children = value;
    mark_dirty();
  }
}

void Widget::set_name(std::string name_) {
  if (name == name_) { return; }
  name = std::move(name_);
}

void Widget::set_marked_for_deletion(bool value) {
  if (marked_for_deletion != value) {
    marked_for_deletion = value;
    mark_dirty();
  }
}
void Widget::set_is_updated(bool value) {
  if (is_updated != value) {
    is_updated = value;
    mark_dirty();
    if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
    if (layout.enabled) { layout_dirty = true; }
  }
}

void Widget::set_is_drawn_on_top(bool value) {
  if (is_drawn_on_top != value) {
    is_drawn_on_top = value;
    mark_dirty();
  }
}

void Widget::set_weight(float value) {
  if (weight != value) {
    weight = value;
    mark_dirty();
    if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
  }
}

void Widget::set_draw_behind_parent(bool value) {
  if (draw_behind_parent != value) {
    draw_behind_parent = value;
    mark_dirty();
  }
}

void Widget::set_is_drawn(bool to) {
  if (is_drawn == to) { return; }
  is_drawn = to;
  mark_dirty();
  if (parent && parent->layout.enabled) { parent->layout_dirty = true; }
}

bool zincgui::Widget::is_focused() const { return ui.focused_widget() == this; }
bool zincgui::Widget::is_focus_manager() const { return ui.focus_manager() == this; }

// void Widget::ui_mark_tree_dirty() { ui.mark_tree_dirty(); }

rect2i Widget::get_rect() const { return rect_; }
