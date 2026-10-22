#include "ui/zincgui/layout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <spanstream>
#include <string>
#include <vector>
#include "common/logger.hpp"

namespace zincgui::layout_engine {

  Layout parse_layout_string(std::string_view def, const Layout& fallback) {
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
      } else if (token == "ltr" || token == "left_to_right" || token == "h" || token == "hor" ||
                 token == "horizontal") {
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
          return fallback;
        }

        if (token.length() <= token_length) {
          out::debug_warn("invalid layout string, missing value for field: {}", token);
          return fallback;
        }

        char* end_ptr = nullptr;
        const char* num_start = token.c_str() + token_length;
        long val = std::strtol(num_start, &end_ptr, 10);

        if (num_start == end_ptr || *end_ptr != '\0') {
          out::debug_warn("invalid layout string, bad integer value: {}", token);
          return fallback;
        }

        if (token_margin) {
          l.margin = vec2i{static_cast<i32>(val), static_cast<i32>(val)};
        } else if (token_margin_x) {
          l.margin.x = static_cast<i32>(val);
        } else if (token_margin_y) {
          l.margin.y = static_cast<i32>(val);
        } else if (token_spacing) {
          l.spacing = static_cast<i32>(val);
        }
      }
    }

    return l;
  }

  void apply_layout(Widget& container) {
    const Layout& layout = container.get_layout();
    if (!layout.enabled) { return; }

    using enum LayoutDirection;
    using enum Anchor;

    i32 child_direction = (layout.direction == LEFT_TO_RIGHT || layout.direction == TOP_TO_BOTTOM) ? 1 : -1;
    bool x_major = (layout.direction == LEFT_TO_RIGHT || layout.direction == RIGHT_TO_LEFT);
    Anchor child_anchor{};

    switch (layout.direction) {
    case LEFT_TO_RIGHT:
      switch (layout.align) {
      case LayoutAlign::BEGIN: {
        child_anchor = TOP_LEFT;
        break;
      }
      case LayoutAlign::CENTER: {
        child_anchor = CENTER_LEFT;
        break;
      }
      case LayoutAlign::END: {
        child_anchor = BOTTOM_LEFT;
        break;
      }
      }
      break;
    case RIGHT_TO_LEFT:
      switch (layout.align) {
      case LayoutAlign::BEGIN: {
        child_anchor = TOP_RIGHT;
        break;
      }
      case LayoutAlign::CENTER: {
        child_anchor = CENTER_RIGHT;
        break;
      }
      case LayoutAlign::END: {
        child_anchor = BOTTOM_RIGHT;
        break;
      }
      }
      break;
    case TOP_TO_BOTTOM:
      switch (layout.align) {
      case LayoutAlign::BEGIN: {
        child_anchor = TOP_LEFT;
        break;
      }
      case LayoutAlign::CENTER: {
        child_anchor = TOP_CENTER;
        break;
      }
      case LayoutAlign::END: {
        child_anchor = TOP_RIGHT;
        break;
      }
      }
      break;
    case BOTTOM_TO_TOP:
      switch (layout.align) {
      case LayoutAlign::BEGIN: {
        child_anchor = BOTTOM_LEFT;
        break;
      }
      case LayoutAlign::CENTER: {
        child_anchor = BOTTOM_CENTER;
        break;
      }
      case LayoutAlign::END: {
        child_anchor = BOTTOM_RIGHT;
        break;
      }
      }
      break;
    }

    i32 offset = x_major ? layout.margin.x : layout.margin.y;
    auto available_space = [&]() -> i32 {
      if (x_major) { return container.get_width() - offset - layout.margin.x; }
      return container.get_height() - offset - layout.margin.y;
    };

    auto set_pos_major = [x_major](Widget& w, i32 to) -> void {
      if (x_major) {
        w.set_x(to);
      } else {
        w.set_y(to);
      }
    };
    auto set_pos_minor = [x_major](Widget& w, i32 to) -> void {
      if (!x_major) {
        w.set_x(to);
      } else {
        w.set_y(to);
      }
    };
    auto get_size_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_width() : w.get_height(); };
    auto get_size_min_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.get_min_width() : w.get_min_height(); };
    auto get_size_max_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.get_max_width() : w.get_max_height(); };
    auto get_size_min_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_min_width() : w.get_min_height(); };
    auto get_size_max_major = [x_major](Widget& w) -> i32 { return x_major ? w.get_max_width() : w.get_max_height(); };
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
    auto get_margin_major = [&layout, x_major]() -> i32 { return x_major ? layout.margin.x : layout.margin.y; };
    auto get_margin_minor = [&layout, x_major]() -> i32 { return !x_major ? layout.margin.x : layout.margin.y; };

    struct FlexItem {
        Widget* w;
        float weight;
        i32 min_size;
        i32 max_size;
        i32 size;
        bool frozen;
    };

    auto& children = container.get_children();
    std::vector<FlexItem> items;
    items.reserve(children.size());

    for (auto& c : children) {
      if (c->get_ignore_parents_layout() || !c->get_is_drawn()) { continue; }
      i32 min_s = get_size_min_major(*c) > 0 ? get_size_min_major(*c) : 0;
      i32 max_s = get_size_max_major(*c) > 0 ? get_size_max_major(*c) : std::numeric_limits<i32>::max();
      if (min_s > max_s) { std::swap(min_s, max_s); }
      float c_weight = layout.fill ? c->get_weight() : 0.0f;
      i32 size = c_weight > 0.0f ? 0 : std::clamp(get_size_major(*c), min_s, max_s);
      items.emplace_back(FlexItem{c.get(), c_weight, min_s, max_s, size, c_weight == 0.0f});
    }

    if (!items.empty()) {
      i32 total_spacing = static_cast<i32>(items.size() - 1) * layout.spacing;
      bool needs_recalc = true;
      while (needs_recalc) {
        needs_recalc = false;
        i32 frozen_space = 0;
        float total_weight = 0.0f;
        for (const auto& item : items) {
          if (item.frozen) {
            frozen_space += item.size;
          } else {
            total_weight += item.weight;
          }
        }
        i32 free_space = available_space() - total_spacing - frozen_space;
        if (total_weight > 0.0f) {
          for (auto& item : items) {
            if (!item.frozen) {
              i32 target = static_cast<i32>(std::round((item.weight / total_weight) * static_cast<float>(free_space)));
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

    i32 container_size_minor = (x_major ? container.get_height() : container.get_width()) - 2 * get_margin_minor();
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
        if (min_s > max_s) { std::swap(min_s, max_s); }
        set_size_minor(*item.w, std::clamp(container_size_minor, min_s, max_s));
      }
    }

    if (!items.empty()) { offset -= layout.spacing; }
    offset += get_margin_major();

    if (layout.fit_to_contents) {
      if (x_major) {
        container.set_width(offset);
      } else {
        container.set_height(offset);
      }
    }
  }

} // namespace zincgui::layout_engine
