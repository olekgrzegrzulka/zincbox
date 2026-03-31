#include "widget.hpp"
#include <glm/vec2.hpp>
#include "common/input.hpp"
#include "ui.hpp"

vec2i Widget::get_position(Anchor relative_to) const {
  vec2i value = anchor_to_uv(parent_anchor) * vec2f{ui.get_window_width(), ui.get_window_height()};
  if (parent) {
    value = parent->get_position(parent_anchor);
  }
  value += vec2f(x, y) - vec2f(width, height) * anchor_to_uv(anchor);
  value += vec2f{width, height} * anchor_to_uv(relative_to);
  return value;
}
void Widget::draw() {
}

void Widget::process_input() {
  for (auto&& child : children) {
    if (child->get_is_updated() && !child->get_marked_for_deletion()) {
      child->process_input();
    }
  }

  for (auto& ev : Input::get_event_queue()) {
    std::visit([&](auto& event) { handle_event(event); }, ev);
  }
}
void Widget::update() {
  std::erase_if(children, [](auto&& child) { return child->get_marked_for_deletion(); });
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

    Anchor child_anchor;
    switch (layout.direction) {
    case LEFT_TO_RIGHT: child_anchor = CENTER_LEFT; break;
    case RIGHT_TO_LEFT: child_anchor = CENTER_RIGHT; break;
    case TOP_TO_BOTTOM: child_anchor = TOP_CENTER; break;
    case BOTTOM_TO_TOP: child_anchor = BOTTOM_CENTER; break;
    }

    float total_weight = 0;
    i32 total_i = 0;
    for (auto& c : children) {
      if (c->ignore_parents_layout) { continue; }
      if (!c->is_drawn) { continue; }
      total_weight += c->get_weight();
      total_i += 1;
    }

    i32 offset = layout.margin;
    auto available_space = [&]() {
      if (x_major) {
        return get_width() - offset - layout.margin;
      } else {
        return get_height() - offset - layout.margin;
      }
    };

    auto set_pos_major = [x_major](Widget& w, i32 to) -> void {
      if (x_major) {
        w.set_x(to);
      } else {
        w.set_y(to);
      }
    };

    auto get_size_major = [x_major](Widget& w) -> i32 {
      if (x_major) {
        return w.get_width();
      } else {
        return w.get_height();
      }
    };

    auto get_size_min_major = [x_major](Widget& w) -> i32 {
      if (x_major) {
        return w.get_min_width();
      } else {
        return w.get_min_height();
      }
    };

    auto get_size_max_major = [x_major](Widget& w) -> i32 {
      if (x_major) {
        return w.get_max_width();
      } else {
        return w.get_max_height();
      }
    };

    auto set_size_major = [x_major](Widget& w, i32 to) -> void {
      if (x_major) {
        w.set_width(to);
      } else {
        w.set_height(to);
      }
    };

    auto set_size_minor = [x_major](Widget& w, i32 to) -> void {
      if (!x_major) {
        w.set_width(to);
      } else {
        w.set_height(to);
      }
    };

    float current_weight = 0;
    i32 i = -1;
    for (auto& c : children) {
      if (c->ignore_parents_layout) { continue; }
      if (!c->is_drawn) { continue; }
      i += 1;
      current_weight += c->get_weight();

      c->set_anchor(child_anchor);
      c->set_parent_anchor(child_anchor);

      if (!layout.fill) {
        set_pos_major(*c, child_direction * offset);
        offset += get_size_major(*c) + layout.spacing;
      } else {
        i32 child_size_major = available_space() / (total_i - i);
        i32 lo = get_size_min_major(*c) > 0 ? get_size_min_major(*c) : std::numeric_limits<i32>::min();
        i32 hi = get_size_max_major(*c) > 0 ? get_size_max_major(*c) : std::numeric_limits<i32>::max();
        child_size_major = std::clamp(child_size_major, lo, hi);
        set_size_major(*c, child_size_major);
        set_pos_major(*c, offset * child_direction);

        offset += child_size_major + layout.spacing;
      }

      if (layout.expand_children) {
        if (x_major) {
          set_size_minor(*c, height - 2 * layout.margin);
        } else {
          set_size_minor(*c, width - 2 * layout.margin);
        }
      }
    }

    offset -= layout.spacing;
    offset += layout.margin;

    if (layout.fit_to_contents) {
      if (x_major) {
        set_width(offset);
      } else {
        set_height(offset);
      }
    }
  }
}

bool Widget::is_mouse_hovering() const {
  return is_mouse_hovering(Input::get_mouse_pos());
}

bool Widget::is_mouse_hovering(vec2i at) const {
  bool mouse_on_widget_x = at.x >= get_position(Anchor::TOP_LEFT).x && at.x < get_position(Anchor::BOTTOM_RIGHT).x;
  bool mouse_on_widget_y = at.y >= get_position(Anchor::TOP_LEFT).y && at.y < get_position(Anchor::BOTTOM_RIGHT).y;
  return mouse_on_widget_x && mouse_on_widget_y;
}

void Widget::set_layout(const std::string& def) {
  Layout l;
  l.enabled = true;

  std::stringstream ss(def);
  std::string token;

  while (ss >> token) {
    if (token == "fit") {
      l.fit_to_contents = true;
    } else if (token == "expand") {
      l.expand_children = true;
    } else if (token == "fill") {
      l.fill = true;
    } else if (token.rfind("m:", 0) == 0 || token.rfind("s:", 0) == 0) {
      if (token.length() <= 2) {
        debug_warn("Missing value for field: ", token);
        return;
      }

      char* end_ptr;
      const char* num_start = token.c_str() + 2;
      long val = std::strtol(num_start, &end_ptr, 10);

      if (num_start == end_ptr || *end_ptr != '\0') {
        debug_warn("Invalid integer value: ", token);
        return;
      }

      if (token[0] == 'm') l.margin = static_cast<i32>(val);
      else l.spacing = static_cast<i32>(val);
    } else if (token == "ltr" || token == "left_to_right" ||
               token == "h" || token == "hor" || token == "horizontal") {
      l.direction = LayoutDirection::LEFT_TO_RIGHT;
    } else if (token == "rtl" || token == "right_to_left") {
      l.direction = LayoutDirection::RIGHT_TO_LEFT;
    } else if (token == "ttb" || token == "top_to_bottom" ||
               token == "v" || token == "ver" || token == "vertical") {
      l.direction = LayoutDirection::TOP_TO_BOTTOM;
    } else if (token == "btt" || token == "bottom_to_top") {
      l.direction = LayoutDirection::BOTTOM_TO_TOP;
    } else {
      debug_warn("Unknown layout keyword: ", token);
      return;
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
    if (layout.enabled) {
      ui.mark_dirty_recursive(this);
    }
  }
}

void Widget::set_size(vec2i size_) {
  if (width != x || height != y) {
    width = size_.x;
    height = size_.y;
    if (layout.enabled) {
      ui.mark_dirty_recursive(this);
    }
  }
}

void Widget::set_width(i32 width_) {
  if (width_ != width) {
    width = width_;
    dirty = true;
    if (layout.enabled) {
      ui.mark_dirty_recursive(this);
    }
  }
}

void Widget::set_height(i32 height_) {
  if (height_ != height) {
    height = height_;
    dirty = true;
    if (layout.enabled) {
      ui.mark_dirty_recursive(this);
    }
  }
}