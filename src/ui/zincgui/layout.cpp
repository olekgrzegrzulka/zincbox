#include "ui/zincgui/layout.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <vector>
#include "common/logger.hpp"

namespace zincgui::layout_engine {

  Layout parse_layout_string(std::string_view def, const Layout& fallback) {
    Layout l;
    l.enabled = true;

    size_t pos = 0;
    while (pos < def.size()) {
      pos = def.find_first_not_of(" \t\r\n", pos);
      if (pos == std::string_view::npos) { break; }

      size_t end = def.find_first_of(" \t\r\n", pos);
      std::string_view token = def.substr(pos, end - pos);
      pos = end;

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
        bool token_margin = token.starts_with("m:");
        bool token_margin_x = token.starts_with("mx:");
        bool token_margin_y = token.starts_with("my:");
        bool token_spacing = token.starts_with("s:");

        size_t prefix_len = 0;
        if (token_margin || token_spacing) {
          prefix_len = 2;
        } else if (token_margin_x || token_margin_y) {
          prefix_len = 3;
        } else {
          out::debug_warn("invalid layout string, bad layout keyword: {}", token);
          return fallback;
        }

        std::string_view num_str = token.substr(prefix_len);
        if (num_str.empty()) {
          out::debug_warn("invalid layout string, missing value for field: {}", token);
          return fallback;
        }

        i32 val = 0;
        auto [ptr, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), val);
        if (ec != std::errc{} || ptr != num_str.data() + num_str.size()) {
          out::debug_warn("invalid layout string, bad integer value: {}", token);
          return fallback;
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

    return l;
  }

  void apply_layout(Widget& container) {
    const Layout& layout = container.layout();
    if (!layout.enabled) { return; }

    using enum LayoutDirection;
    using enum Anchor;

    static constexpr Anchor ANCHOR_LUT[4][3] = {{TOP_LEFT, CENTER_LEFT, BOTTOM_LEFT},
                                                {TOP_RIGHT, CENTER_RIGHT, BOTTOM_RIGHT},
                                                {TOP_LEFT, TOP_CENTER, TOP_RIGHT},
                                                {BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT}};

    Anchor child_anchor = ANCHOR_LUT[(size_t)(layout.direction)][(size_t)(layout.align)];
    i32 child_direction = (layout.direction == LEFT_TO_RIGHT || layout.direction == TOP_TO_BOTTOM) ? 1 : -1;
    bool x_major = (layout.direction == LEFT_TO_RIGHT || layout.direction == RIGHT_TO_LEFT);

    i32 margin_major = x_major ? layout.margin.x : layout.margin.y;
    i32 margin_minor = x_major ? layout.margin.y : layout.margin.x;
    i32 container_size_major = x_major ? container.width() : container.height();
    i32 container_size_minor = (x_major ? container.height() : container.width()) - (2 * margin_minor);

    auto set_pos_major = [x_major](Widget& w, i32 to) -> void {
      if (x_major) {
        w.x(to, true);
      } else {
        w.y(to, true);
      }
    };
    auto set_pos_minor = [x_major](Widget& w, i32 to) -> void {
      if (!x_major) {
        w.x(to, true);
      } else {
        w.y(to, true);
      }
    };
    auto get_size_major = [x_major](Widget& w) -> i32 { return x_major ? w.width() : w.height(); };
    auto get_size_min_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.min_width() : w.min_height(); };
    auto get_size_max_minor = [x_major](Widget& w) -> i32 { return !x_major ? w.max_width() : w.max_height(); };
    auto get_size_min_major = [x_major](Widget& w) -> i32 { return x_major ? w.min_width() : w.min_height(); };
    auto get_size_max_major = [x_major](Widget& w) -> i32 { return x_major ? w.max_width() : w.max_height(); };
    auto set_size_major = [x_major](Widget& w, i32 to) -> void {
      if (x_major) {
        w.width(to, true);
      } else {
        w.height(to, true);
      }
    };
    auto set_size_minor = [x_major](Widget& w, i32 to) -> void {
      if (!x_major) {
        w.width(to, true);
      } else {
        w.height(to, true);
      }
    };

    struct FlexItem {
        Widget* w;
        float weight;
        i32 min_size;
        i32 max_size;
        i32 size;
        bool frozen;
    };

    auto& children = container.children();
    static std::vector<FlexItem> items;
    items.clear();
    items.reserve(children.size());

    i32 frozen_space = 0;
    float total_weight = 0.0f;

    for (auto& c : children) {
      if (c->ignore_parents_layout() || !c->is_drawn()) { continue; }
      i32 raw_min = get_size_min_major(*c);
      i32 raw_max = get_size_max_major(*c);
      i32 min_s = raw_min > 0 ? raw_min : 0;
      i32 max_s = raw_max > 0 ? raw_max : std::numeric_limits<i32>::max();
      if (min_s > max_s) { std::swap(min_s, max_s); }
      float c_weight = layout.fill ? c->weight() : 0.0f;
      bool is_frozen = (c_weight <= 0.0f);
      i32 size = is_frozen ? std::clamp(get_size_major(*c), min_s, max_s) : 0;

      if (is_frozen) {
        frozen_space += size;
      } else {
        total_weight += c_weight;
      }

      items.emplace_back(FlexItem{c.get(), c_weight, min_s, max_s, size, is_frozen});
    }

    if (!items.empty() && total_weight > 0.0f) {
      i32 total_spacing = static_cast<i32>(items.size() - 1) * layout.spacing;
      i32 usable_space = container_size_major - (2 * margin_major) - total_spacing;
      bool needs_recalc = true;

      while (needs_recalc && total_weight > 0.0f) {
        needs_recalc = false;
        i32 free_space = usable_space - frozen_space;
        float weight_factor = static_cast<float>(free_space) / total_weight;

        for (auto& item : items) {
          if (!item.frozen) {
            i32 target = static_cast<i32>(std::lround(item.weight * weight_factor));
            if (target <= item.min_size) {
              item.size = item.min_size;
              item.frozen = true;
              frozen_space += item.size;
              total_weight -= item.weight;
              needs_recalc = true;
            } else if (target >= item.max_size) {
              item.size = item.max_size;
              item.frozen = true;
              frozen_space += item.size;
              total_weight -= item.weight;
              needs_recalc = true;
            } else {
              item.size = target;
            }
          }
        }
      }
    }

    i32 offset = margin_major;
    i32 offset_minor = (layout.align == LayoutAlign::CENTER) ? 0 : margin_minor;

    for (auto& item : items) {
      Widget& w = *item.w;
      w.anchor(child_anchor, true).parent_anchor(child_anchor, true);
      set_size_major(w, item.size);
      set_pos_major(w, child_direction * offset);
      offset += item.size + layout.spacing;
      set_pos_minor(w, offset_minor);

      if (layout.expand_children) {
        i32 raw_min = get_size_min_minor(w);
        i32 raw_max = get_size_max_minor(w);
        i32 min_s = raw_min > 0 ? raw_min : 0;
        i32 max_s = raw_max > 0 ? raw_max : std::numeric_limits<i32>::max();
        if (min_s > max_s) { std::swap(min_s, max_s); }
        set_size_minor(w, std::clamp(container_size_minor, min_s, max_s));
      }
    }

    if (!items.empty()) { offset -= layout.spacing; }
    offset += margin_major;

    if (layout.fit_to_contents) {
      if (x_major) {
        i32 min_w = container.min_width() > 0 ? container.min_width() : 0;
        i32 max_w = container.max_width() > 0 ? container.max_width() : std::numeric_limits<i32>::max();
        container.width(std::clamp(offset, min_w, max_w), true);
      } else {
        i32 min_h = container.min_height() > 0 ? container.min_height() : 0;
        i32 max_h = container.max_height() > 0 ? container.max_height() : std::numeric_limits<i32>::max();
        container.height(std::clamp(offset, min_h, max_h), true);
      }
    }
  }

} // namespace zincgui::layout_engine
