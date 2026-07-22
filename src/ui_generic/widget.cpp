#include "widget.hpp"
#include <spanstream>
#include <glm/vec2.hpp>
#include "common/input.hpp"
#include "ui.hpp"

vec2i Widget::get_position(Anchor relative_to) const {
  vec2i value = anchor_to_uv(parent_anchor) * vec2f{ui.get_window_size()};
  if (parent) { value = parent->get_position(parent_anchor); }
  value += vec2f(x, y) - vec2f(width, height) * anchor_to_uv(anchor);
  value += vec2f{width, height} * anchor_to_uv(relative_to);
  return value;
}
void Widget::draw() {}

void Widget::input() {}

void Widget::update() {
#ifdef WIDGET_DRAW_DEBUG_RECT
  if (!is_debug_rect) {
    if (!debug_rect) {
      debug_rect = &add_child<Sprite>();
      debug_rect->is_debug_rect = true;
      debug_rect->ignore_parents_layout = true;
      ((Sprite*)debug_rect)->set_texture("red");
      ((Sprite*)debug_rect)->set_nine_slice_margin(1);
      ((Sprite*)debug_rect)->set_nine_slice_scale(1);
    }

    ((Sprite*)debug_rect)->set_size(width, height);
  }
#endif

  if (layout.enabled) {
    using enum LayoutDirection;
    using enum Anchor;
    i32 child_direction = (layout.direction == LEFT_TO_RIGHT || layout.direction == TOP_TO_BOTTOM) ? 1 : -1;
    bool x_major = (layout.direction == LEFT_TO_RIGHT || layout.direction == RIGHT_TO_LEFT);
    Anchor child_anchor{};
    switch (layout.direction) {
    case LEFT_TO_RIGHT:
      switch (layout.align) {
      case LayoutAlign::BEGIN: child_anchor = TOP_LEFT; break;
      case LayoutAlign::CENTER: child_anchor = CENTER_LEFT; break;
      case LayoutAlign::END: child_anchor = BOTTOM_LEFT; break;
      }
      break;
    case RIGHT_TO_LEFT:
      switch (layout.align) {
      case LayoutAlign::BEGIN: child_anchor = TOP_RIGHT; break;
      case LayoutAlign::CENTER: child_anchor = CENTER_RIGHT; break;
      case LayoutAlign::END: child_anchor = BOTTOM_RIGHT; break;
      }
      break;
    case TOP_TO_BOTTOM:
      switch (layout.align) {
      case LayoutAlign::BEGIN: child_anchor = TOP_LEFT; break;
      case LayoutAlign::CENTER: child_anchor = TOP_CENTER; break;
      case LayoutAlign::END: child_anchor = TOP_RIGHT; break;
      }
      break;
    case BOTTOM_TO_TOP:
      switch (layout.align) {
      case LayoutAlign::BEGIN: child_anchor = BOTTOM_LEFT; break;
      case LayoutAlign::CENTER: child_anchor = BOTTOM_CENTER; break;
      case LayoutAlign::END: child_anchor = BOTTOM_RIGHT; break;
      }
      break;
    }
    i32 offset = x_major ? layout.margin.x : layout.margin.y;
    auto available_space = [&]() -> i32 {
      if (x_major) { return get_width() - offset - layout.margin.x; }
      return get_height() - offset - layout.margin.y;
    };
    auto set_pos_major = [x_major](Widget& w, i32 to) -> void { x_major ? w.set_x(to) : w.set_y(to); };
    auto set_pos_minor = [x_major](Widget& w, i32 to) -> void { !x_major ? w.set_x(to) : w.set_y(to); };
    auto get_size_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_width() : w.get_height(); };
    auto get_size_min_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.get_min_width() : w.get_min_height(); };
    auto get_size_max_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.get_max_width() : w.get_max_height(); };
    auto get_size_min_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_min_width() : w.get_min_height(); };
    auto get_size_max_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_max_width() : w.get_max_height(); };
    auto set_size_major = [x_major](Widget& w, i32 to) -> void { x_major ? w.set_width(to) : w.set_height(to); };
    auto set_size_minor = [x_major](Widget& w, i32 to) -> void { !x_major ? w.set_width(to) : w.set_height(to); };
    auto get_margin_major = [this, x_major]() -> i32 { return x_major ? layout.margin.x : layout.margin.y; };
    auto get_margin_minor = [this, x_major]() -> i32 { return !x_major ? layout.margin.x : layout.margin.y; };
    struct FlexItem {
        Widget* w;
        float weight;
        i32 min_size;
        i32 max_size;
        i32 size;
        bool frozen;
    };
    std::vector<FlexItem> items;
    items.reserve(children.size());
    i32 total_spacing = 0;
    for (auto& c : children) {
      if (c->ignore_parents_layout || !c->is_drawn) { continue; }
      i32 min_s = get_size_min_major(*c) > 0 ? get_size_min_major(*c) : 0;
      i32 max_s = get_size_max_major(*c) > 0 ? get_size_max_major(*c) : std::numeric_limits<i32>::max();
      float c_weight = layout.fill ? c->weight : 0.0f;
      i32 size = c_weight > 0 ? 0 : std::clamp(get_size_major(*c), min_s, max_s);
      items.emplace_back(FlexItem{&*c, c_weight, min_s, max_s, size, c_weight == 0});
    }

    if (!items.empty()) {
      total_spacing = (items.size() - 1) * layout.spacing;
      bool needs_recalc = true;
      while (needs_recalc) {
        needs_recalc = false;
        i32 frozen_space = 0;
        float total_weight = 0;
        for (const auto& item : items) {
          if (item.frozen) {
            frozen_space += item.size;
          } else {
            total_weight += item.weight;
          }
        }
        i32 free_space = available_space() - total_spacing - frozen_space;
        if (total_weight > 0) {
          for (auto& item : items) {
            if (!item.frozen) {
              i32 target = std::round((item.weight / total_weight) * free_space);
              if (target <= item.min_size) {
                item.size = item.min_size;
                item.frozen = true;
                needs_recalc = true;
              } else if (target >= item.max_size) {
                item.size = item.max_size;
                item.frozen = true;
                needs_recalc = true;
              } else {
                item.size = target;
              }
            }
          }
        }
      }
    }
    i32 container_size_minor = (x_major ? height : width) - 2 * get_margin_minor();
    for (auto& item : items) {
      item.w->set_anchor(child_anchor);
      item.w->set_parent_anchor(child_anchor);
      set_size_major(*item.w, item.size);
      set_pos_major(*item.w, child_direction * offset);
      offset += item.size + layout.spacing;
      i32 offset_minor = layout.align == LayoutAlign::CENTER ? 0 : get_margin_minor();
      set_pos_minor(*item.w, offset_minor);
      if (layout.expand_children) {
        i32 min_s = get_size_min_minor(*item.w) > 0 ? get_size_min_minor(*item.w) : 0;
        i32 max_s = get_size_max_minor(*item.w) > 0 ? get_size_max_minor(*item.w) : std::numeric_limits<i32>::max();
        set_size_minor(*item.w, std::clamp(container_size_minor, min_s, max_s));
      }
    }
    offset -= layout.spacing;
    offset += get_margin_major();
    if (layout.fit_to_contents) {
      if (x_major) {
        set_width(offset);
      } else {
        set_height(offset);
      }
    }
  }
}

bool Widget::is_mouse_hovering() const { return is_mouse_hovering(Input::get_mouse_pos()); }

bool Widget::is_mouse_hovering(vec2i at) const {
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

void Widget::set_layout(std::string_view def) {
  Layout l;
  l.enabled = true;

  std::ispanstream ss(def);
  std::string token;

  while (ss >> token) {
    if (token == "fit") {
      l.fit_to_contents = true;
    } else if (token == "expand") {
      l.expand_children = true;
    } else if (token == "fill") {
      l.fill = true;
    } else if (token == "ltr" || token == "left_to_right" || token == "h" || token == "hor" || token == "horizontal") {
      l.direction = LayoutDirection::LEFT_TO_RIGHT;
    } else if (token == "rtl" || token == "right_to_left") {
      l.direction = LayoutDirection::RIGHT_TO_LEFT;
    } else if (token == "ttb" || token == "top_to_bottom" || token == "v" || token == "ver" || token == "vertical") {
      l.direction = LayoutDirection::TOP_TO_BOTTOM;
    } else if (token == "btt" || token == "bottom_to_top") {
      l.direction = LayoutDirection::BOTTOM_TO_TOP;
    } else if (token == "left" || token == "top" || token == "begin" || token == "start") {
      l.align = LayoutAlign::BEGIN;
    } else if (token == "right" || token == "bottom" || token == "end") {
      l.align = LayoutAlign::END;
    } else if (token == "center" || token == "middle") {
      l.align = LayoutAlign::CENTER;
    } else {
      bool token_margin = token.rfind("m:", 0) == 0;
      bool token_margin_x = token.rfind("mx:", 0) == 0;
      bool token_margin_y = token.rfind("my:", 0) == 0;
      bool token_spacing = token.rfind("s:", 0) == 0;

      size_t token_length = 0;
      if (token_margin || token_spacing) {
        token_length = 2;
      } else if (token_margin_x || token_margin_y) {
        token_length = 3;
      } else {
        out::debug_warn("invalid layout string, bad layout keyword: {}", token);
        return;
      }

      if (token.length() <= token_length) {
        out::debug_warn("invalid layout string, missing value for field: {}", token);
        return;
      }

      char* end_ptr;
      const char* num_start = token.c_str() + token_length;
      long val = std::strtol(num_start, &end_ptr, 10);

      if (num_start == end_ptr || *end_ptr != '\0') {
        out::debug_warn("invalid layout string, bad integer value: {}", token);
        return;
      }

      if (token_margin) {
        l.margin = vec2i{val, val};
      } else if (token_margin_x) {
        l.margin.x = val;
      } else if (token_margin_y) {
        l.margin.y = val;
      } else if (token_spacing) {
        l.spacing = val;
      }
    }
  }

  if (layout != l) {
    mark_dirty();
    layout = l;
  }
}

void Widget::set_pos(i32 x_, i32 y_) {
  if (x_ != x || y_ != y) {
    x = x_;
    y = y_;
    ui.mark_dirty_recursive(this);
  }
}

void Widget::set_pos(vec2i pos) {
  if (pos.x != x || pos.y != y) {
    x = pos.x;
    y = pos.y;
    ui.mark_dirty_recursive(this);
  }
}

void Widget::set_size(i32 w_, i32 h_) {
  if (w_ != width || h_ != height) {
    width = w_;
    height = h_;
    if (layout.enabled) { ui.mark_dirty_recursive(this); }
  }
}

void Widget::set_size(vec2i size_) {
  if (width != x || height != y) {
    width = size_.x;
    height = size_.y;
    if (layout.enabled) { ui.mark_dirty_recursive(this); }
  }
}

void Widget::set_width(i32 width_) {
  if (width_ != width) {
    width = width_;
    dirty = true;
    if (layout.enabled) { ui.mark_dirty_recursive(this); }
  }
}

void Widget::set_height(i32 height_) {
  if (height_ != height) {
    height = height_;
    dirty = true;
    if (layout.enabled) { ui.mark_dirty_recursive(this); }
  }
}
