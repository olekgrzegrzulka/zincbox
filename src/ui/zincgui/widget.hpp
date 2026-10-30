#pragma once
#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stacktrace>
#include <string>
#include <type_traits>
#include <vector>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/vertex.hpp"

namespace zincgui {

  class Root;

  enum class LayoutDirection : u8 { LEFT_TO_RIGHT, RIGHT_TO_LEFT, TOP_TO_BOTTOM, BOTTOM_TO_TOP };

  enum class LayoutAlign : u8 { BEGIN, CENTER, END };

  enum class Anchor : u8 {
    TOP_LEFT = 0,
    TOP_CENTER = 1,
    TOP_RIGHT = 2,
    CENTER_LEFT = 3,
    CENTER_CENTER = 4,
    CENTER_RIGHT = 5,
    BOTTOM_LEFT = 6,
    BOTTOM_CENTER = 7,
    BOTTOM_RIGHT = 8,

    LEFT = CENTER_LEFT,
    RIGHT = CENTER_RIGHT,
    TOP = TOP_CENTER,
    BOTTOM = BOTTOM_CENTER,
    CENTER = CENTER_CENTER,
  };

  static constexpr vec2f ANCHOR_UV_LUT[9] = {
    {0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.5f},
    {1.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f},
  };

  enum class HitTestBehavior : u8 {
    IGNORE,
    BLOCK,
    PASS_THROUGH,
  };

  struct Layout {
      bool enabled = false;
      bool fit_to_contents = false;
      bool expand_children = false;
      bool fill = false;
      vec2i margin{};
      i32 spacing = 0;
      LayoutDirection direction = LayoutDirection::LEFT_TO_RIGHT;
      LayoutAlign align = LayoutAlign::CENTER;

      void set_margin(i32 value) { margin = vec2i{value, value}; }

      bool operator==(const Layout&) const = default;
  };

  inline std::string anchor_to_string(Anchor anchor) {
    using enum Anchor;
    switch (anchor) {
    case TOP_LEFT: return "TOP_LEFT";
    case TOP_CENTER: return "TOP_CENTER";
    case TOP_RIGHT: return "TOP_RIGHT";
    case CENTER_LEFT: return "CENTER_LEFT";
    case CENTER_CENTER: return "CENTER_CENTER";
    case CENTER_RIGHT: return "CENTER_RIGHT";
    case BOTTOM_LEFT: return "BOTTOM_LEFT";
    case BOTTOM_CENTER: return "BOTTOM_CENTER";
    case BOTTOM_RIGHT: return "BOTTOM_RIGHT";
    }
    ensure(false);
    return "";
  }

  ZINCBOX_INLINE static vec2f anchor_to_uv(Anchor anchor) noexcept { return ANCHOR_UV_LUT[static_cast<u8>(anchor)]; }

  namespace Input {
    struct InputEventMouseButton;
    struct InputEventMouseMove;
    struct InputEventMouseScroll;
    struct InputEventKey;
    struct InputEventMouseEntered;
    struct InputEventMouseLeft;
    struct InputEventCloseWindow;
  } // namespace Input

  class Widget {
      friend Root;

    public:
      Widget(Root& ui_);
      Widget(Root& ui_, i32 width_, i32 height_);
      virtual ~Widget();

      virtual void input();
      virtual void update();
      virtual void on_hovered() {}
      virtual void on_unhovered() {}
      virtual void on_focused() {}
      virtual void on_unfocused() {}
      [[nodiscard]] virtual std::span<const Vertex> vertices();

      virtual void event(Input::InputEventMouseButton&) {}
      virtual void event(Input::InputEventMouseMove&) {}
      virtual void event(Input::InputEventMouseScroll&) {}
      virtual void event(Input::InputEventKey&) {}
      virtual void event(Input::InputEventMouseEntered&) {}
      virtual void event(Input::InputEventMouseLeft&) {}
      virtual void event(Input::InputEventCloseWindow&) {}

      template <class T, class... Args> T& add_child(Args&&... args) {
        static_assert(std::is_base_of_v<Widget, T>);
        m_children_to_add.emplace_back(std::make_unique<T>(m_ui, std::forward<Args>(args)...));
        T& widget = static_cast<T&>(*m_children_to_add.back().get());
        widget.m_parent = this;
        mark_dirty();
        return widget;
      }

      [[nodiscard]] bool is_mouse_hovering() const;
      [[nodiscard]] bool is_mouse_hovering(vec2i at) const;
      [[nodiscard]] bool is_focused() const;
      [[nodiscard]] bool is_focus_manager() const;
      [[nodiscard]] bool has_layout() const { return m_layout.enabled; }

      void mark_layout_dirty();
      void mark_dirty();

      [[nodiscard]] Root& ui() noexcept { return m_ui; }
      [[nodiscard]] const Root& ui() const noexcept { return m_ui; }

      [[nodiscard]] const std::string& name() const noexcept { return m_name; }
      template <typename Self> Self& name(this Self& self, std::string name_) {
        if (self.m_name != name_) { self.m_name = std::move(name_); }
        return self;
      }

      [[nodiscard]] i32 x() const noexcept { return m_x; }
      template <typename Self> Self& x(this Self& self, i32 x_, bool ignore_constraints = false) {
        if (self.m_x == x_) { return self; }
        if (!ignore_constraints && !self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
          return self;
        }
        self.m_x = x_;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] i32 y() const noexcept { return m_y; }
      template <typename Self> Self& y(this Self& self, i32 y_, bool ignore_constraints = false) {
        if (self.m_y == y_) { return self; }
        if (!ignore_constraints && !self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
          return self;
        }
        self.m_y = y_;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] i32 width() const noexcept {
        i32 lo = self_min_width();
        i32 hi = self_max_width();
        if (lo > hi) { std::swap(lo, hi); }
        return std::clamp(m_width, lo, hi);
      }
      template <typename Self> Self& width(this Self& self, i32 width_, bool ignore_constraints = false) {
        if (self.m_width == width_) { return self; }
        if (!ignore_constraints) {
          if (self.m_layout.enabled && self.m_layout.fit_to_contents &&
              (self.m_layout.direction == LayoutDirection::LEFT_TO_RIGHT ||
               self.m_layout.direction == LayoutDirection::RIGHT_TO_LEFT)) {
            return self;
          }
          if (!self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
            if (self.m_parent->m_layout.expand_children &&
                (self.m_parent->m_layout.direction == LayoutDirection::TOP_TO_BOTTOM ||
                 self.m_parent->m_layout.direction == LayoutDirection::BOTTOM_TO_TOP)) {
              return self;
            }
            if (self.m_parent->m_layout.fill && self.m_weight > 0.0f &&
                (self.m_parent->m_layout.direction == LayoutDirection::LEFT_TO_RIGHT ||
                 self.m_parent->m_layout.direction == LayoutDirection::RIGHT_TO_LEFT)) {
              return self;
            }
          }
        }
        self.m_width = width_;
        self.mark_dirty();
        if (self.m_layout.enabled && (self.m_layout.expand_children || self.m_layout.fill)) {
          self.mark_layout_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 height() const noexcept {
        i32 lo = self_min_height();
        i32 hi = self_max_height();
        if (lo > hi) { std::swap(lo, hi); }
        return std::clamp(m_height, lo, hi);
      }
      template <typename Self> Self& height(this Self& self, i32 height_, bool ignore_constraints = false) {
        if (self.m_height == height_) { return self; }
        if (!ignore_constraints) {
          if (self.m_layout.enabled && self.m_layout.fit_to_contents &&
              (self.m_layout.direction == LayoutDirection::TOP_TO_BOTTOM ||
               self.m_layout.direction == LayoutDirection::BOTTOM_TO_TOP)) {
            return self;
          }
          if (!self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
            if (self.m_parent->m_layout.expand_children &&
                (self.m_parent->m_layout.direction == LayoutDirection::LEFT_TO_RIGHT ||
                 self.m_parent->m_layout.direction == LayoutDirection::RIGHT_TO_LEFT)) {
              return self;
            }
            if (self.m_parent->m_layout.fill && self.m_weight > 0.0f &&
                (self.m_parent->m_layout.direction == LayoutDirection::TOP_TO_BOTTOM ||
                 self.m_parent->m_layout.direction == LayoutDirection::BOTTOM_TO_TOP)) {
              return self;
            }
          }
        }
        self.m_height = height_;
        self.mark_dirty();
        if (self.m_layout.enabled && (self.m_layout.expand_children || self.m_layout.fill)) {
          self.mark_layout_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 min_width() const noexcept { return m_min_width; }
      template <typename Self> Self& min_width(this Self& self, i32 v) {
        if (self.m_min_width == v) { return self; }
        self.m_min_width = v;
        self.mark_dirty();
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        return self;
      }

      [[nodiscard]] i32 max_width() const noexcept { return m_max_width; }
      template <typename Self> Self& max_width(this Self& self, i32 v) {
        if (self.m_max_width == v) { return self; }
        self.m_max_width = v;
        self.mark_dirty();
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        return self;
      }

      [[nodiscard]] i32 min_height() const noexcept { return m_min_height; }
      template <typename Self> Self& min_height(this Self& self, i32 v) {
        if (self.m_min_height == v) { return self; }
        self.m_min_height = v;
        self.mark_dirty();
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        return self;
      }

      [[nodiscard]] i32 max_height() const noexcept { return m_max_height; }
      template <typename Self> Self& max_height(this Self& self, i32 v) {
        if (self.m_max_height == v) { return self; }
        self.m_max_height = v;
        self.mark_dirty();
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        return self;
      }

      template <typename Self> Self& fixed_width(this Self& self, i32 value) {
        self.min_width(value).max_width(value);
        return self;
      }

      template <typename Self> Self& fixed_height(this Self& self, i32 value) {
        self.min_height(value).max_height(value);
        return self;
      }

      [[nodiscard]] vec2i size() const noexcept { return vec2i(m_width, m_height); }
      template <typename Self> Self& size(this Self& self, i32 w, i32 h) {
        self.width(w).height(h);
        return self;
      }
      template <typename Self> Self& size(this Self& self, vec2i s) {
        self.width(s.x).height(s.y);
        return self;
      }

      template <typename Self> Self& fixed_size(this Self& self, i32 w, i32 h) {
        self.fixed_width(w).fixed_height(h);
        return self;
      }
      template <typename Self> Self& fixed_size(this Self& self, vec2i s) {
        self.fixed_width(s.x).fixed_height(s.y);
        return self;
      }

      [[nodiscard]] vec2i min_size() const noexcept { return vec2i(m_min_width, m_min_height); }
      [[nodiscard]] vec2i max_size() const noexcept { return vec2i(m_max_width, m_max_height); }

      [[nodiscard]] vec2i pos() const noexcept { return vec2i(m_x, m_y); }
      template <typename Self> Self& pos(this Self& self, i32 x_, i32 y_) {
        self.x(x_).y(y_);
        return self;
      }
      template <typename Self> Self& pos(this Self& self, vec2i p) {
        self.x(p.x).y(p.y);
        return self;
      }

      [[nodiscard]] Anchor anchor() const noexcept { return m_anchor; }
      template <typename Self> Self& anchor(this Self& self, Anchor a, bool ignore_constraints = false) {
        if (self.m_anchor == a) { return self; }
        if (!ignore_constraints && !self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
          return self;
        }
        self.m_anchor = a;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] Anchor parent_anchor() const noexcept { return m_parent_anchor; }
      template <typename Self> Self& parent_anchor(this Self& self, Anchor a, bool ignore_constraints = false) {
        if (self.m_parent_anchor == a) { return self; }
        if (!ignore_constraints && !self.m_ignore_parents_layout && self.m_parent && self.m_parent->m_layout.enabled) {
          return self;
        }
        self.m_parent_anchor = a;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] ZINCBOX_INLINE vec2i position(Anchor relative_to = Anchor::TOP_LEFT) const noexcept {
        static constexpr std::pair<u8, u8> ANCHOR_SCALES[9] = {{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1},
                                                               {2, 1}, {0, 2}, {1, 2}, {2, 2}};
        const auto [sx, sy] = ANCHOR_SCALES[static_cast<u8>(relative_to)];
        return {m_global_position.x + ((m_width * sx) >> 1), m_global_position.y + ((m_height * sy) >> 1)};
      }

      [[nodiscard]] rect2i rect() const noexcept { return m_rect; }

      [[nodiscard]] bool dirty() const noexcept { return m_dirty; }
      template <typename Self> Self& dirty(this Self& self, bool d) {
        if (self.m_dirty != d) { self.m_dirty = d; }
        return self;
      }

      [[nodiscard]] bool marked_for_deletion() const noexcept { return m_marked_for_deletion; }
      [[nodiscard]] bool is_marked_for_deletion() const noexcept { return m_marked_for_deletion; }
      template <typename Self> Self& marked_for_deletion(this Self& self, bool value) {
        if (self.m_marked_for_deletion != value) {
          self.m_marked_for_deletion = value;
          self.mark_dirty();
        }
        return self;
      }
      template <typename Self> Self& is_marked_for_deletion(this Self& self, bool value) {
        return self.marked_for_deletion(value);
      }

      [[nodiscard]] bool is_updated() const noexcept { return m_is_updated; }
      template <typename Self> Self& is_updated(this Self& self, bool value) {
        if (self.m_is_updated != value) {
          self.m_is_updated = value;
          self.mark_dirty();
          if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
          if (self.m_layout.enabled) { self.mark_layout_dirty(); }
        }
        return self;
      }

      [[nodiscard]] bool is_drawn() const noexcept { return m_is_drawn; }
      template <typename Self> Self& is_drawn(this Self& self, bool to) {
        if (self.m_is_drawn == to) { return self; }
        self.m_is_drawn = to;
        self.mark_dirty();
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        return self;
      }

      [[nodiscard]] bool is_self_drawn() const noexcept { return m_is_self_drawn; }
      template <typename Self> Self& is_self_drawn(this Self& self, bool value) {
        if (self.m_is_self_drawn != value) {
          self.m_is_self_drawn = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool clip() const noexcept { return m_clip; }
      template <typename Self> Self& clip(this Self& self, bool value) {
        if (self.m_clip != value) {
          self.m_clip = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool clip_children() const noexcept { return m_clip_children; }
      template <typename Self> Self& clip_children(this Self& self, bool value) {
        if (self.m_clip_children != value) {
          self.m_clip_children = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool is_focusable() const noexcept { return m_is_focusable; }
      template <typename Self> Self& is_focusable(this Self& self, bool value) {
        if (self.m_is_focusable != value) {
          self.m_is_focusable = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool ignore_parents_layout() const noexcept { return m_ignore_parents_layout; }
      template <typename Self> Self& ignore_parents_layout(this Self& self, bool to) {
        if (self.m_ignore_parents_layout == to) { return self; }
        self.m_ignore_parents_layout = to;
        if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] bool is_drawn_on_top() const noexcept { return m_is_drawn_on_top; }
      template <typename Self> Self& is_drawn_on_top(this Self& self, bool value) {
        if (self.m_is_drawn_on_top != value) {
          self.m_is_drawn_on_top = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float weight() const noexcept { return m_weight; }
      template <typename Self> Self& weight(this Self& self, float value) {
        if (self.m_weight != value) {
          self.m_weight = value;
          self.mark_dirty();
          if (self.m_parent && self.m_parent->m_layout.enabled) { self.m_parent->mark_layout_dirty(); }
        }
        return self;
      }

      [[nodiscard]] bool draw_behind_parent() const noexcept { return m_draw_behind_parent; }
      template <typename Self> Self& draw_behind_parent(this Self& self, bool value) {
        if (self.m_draw_behind_parent != value) {
          self.m_draw_behind_parent = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] HitTestBehavior hit_test_behavior() const noexcept { return m_hit_test_behavior; }
      template <typename Self> Self& hit_test_behavior(this Self& self, HitTestBehavior value) {
        if (self.m_hit_test_behavior != value) {
          self.m_hit_test_behavior = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool hover_test_parent() const noexcept { return m_hover_test_parent; }
      template <typename Self> Self& hover_test_parent(this Self& self, bool to) {
        if (self.m_hover_test_parent != to) { self.m_hover_test_parent = to; }
        return self;
      }

      [[nodiscard]] bool update_children_first() const noexcept { return m_update_children_first; }
      template <typename Self> Self& update_children_first(this Self& self, bool value) {
        if (self.m_update_children_first != value) {
          self.m_update_children_first = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 window_width() const noexcept { return m_window_width; }
      template <typename Self> Self& window_width(this Self& self, i32 w) {
        if (self.m_window_width != w) {
          self.m_window_width = w;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 window_height() const noexcept { return m_window_height; }
      template <typename Self> Self& window_height(this Self& self, i32 h) {
        if (self.m_window_height != h) {
          self.m_window_height = h;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] Widget* parent() noexcept { return m_parent; }
      [[nodiscard]] const Widget* parent() const noexcept { return m_parent; }
      template <typename Self> Self& parent(this Self& self, Widget* p) noexcept {
        self.m_parent = p;
        return self;
      }

      [[nodiscard]] std::vector<std::unique_ptr<Widget>>& children() noexcept { return m_children; }
      [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const noexcept { return m_children; }

      [[nodiscard]] Layout& layout() noexcept { return m_layout; }
      [[nodiscard]] const Layout& layout() const noexcept { return m_layout; }
      template <typename Self> Self& layout(this Self& self, std::string_view def) {
        self.set_layout_impl(def);
        return self;
      }
      [[nodiscard]] bool is_layout_dirty() const noexcept { return m_layout_dirty; }

    private:
      [[nodiscard]] i32 self_min_width() const noexcept {
        return m_min_width > 0 ? m_min_width : std::numeric_limits<i32>::min();
      }
      [[nodiscard]] i32 self_max_width() const noexcept {
        return m_max_width > 0 ? m_max_width : std::numeric_limits<i32>::max();
      }
      [[nodiscard]] i32 self_min_height() const noexcept {
        return m_min_height > 0 ? m_min_height : std::numeric_limits<i32>::min();
      }
      [[nodiscard]] i32 self_max_height() const noexcept {
        return m_max_height > 0 ? m_max_height : std::numeric_limits<i32>::max();
      }
      void set_layout_impl(std::string_view def);

    private:
      Root& m_ui;
      std::vector<std::unique_ptr<Widget>> m_children_to_add;
      std::string m_name;
      i32 m_x = 0;
      i32 m_y = 0;
      bool m_hover_test_parent = true;
      i32 m_width = default_settings::widget_size.x;
      i32 m_height = default_settings::widget_size.y;
      i32 m_min_width = 0;
      i32 m_max_width = 0;
      i32 m_min_height = 0;
      i32 m_max_height = 0;
      Anchor m_anchor = Anchor::TOP_LEFT;
      Anchor m_parent_anchor = Anchor::TOP_LEFT;
      bool m_dirty = true;
      bool m_marked_for_deletion = false;
      bool m_is_updated = true;
      bool m_is_drawn = true;
      bool m_is_self_drawn = true;
      bool m_clip = false;
      bool m_clip_children = false;
      bool m_is_focusable = false;
      bool m_ignore_parents_layout = false;
      bool m_is_drawn_on_top = false;
      float m_weight = default_settings::widget_weight;
      bool m_draw_behind_parent = false;
      HitTestBehavior m_hit_test_behavior = HitTestBehavior::PASS_THROUGH;
      bool m_hovered = false;
      bool m_visible = false;
      bool m_updated = false;
      u64 m_last_update_frame = 0;
      bool m_deleted = false;
      i32 m_always_on_top_layer = 0;
      std::optional<rect2i> m_scissor;
      rect2i m_rect;
      rect2i m_hit_test_rect;
      vec2i m_global_position{};
      Widget* m_parent = nullptr;
      std::vector<std::unique_ptr<Widget>> m_children;
      bool m_update_children_first = false;
      Layout m_layout;
      bool m_layout_dirty = false;
      i32 m_window_width = 0;
      i32 m_window_height = 0;
#ifdef WIDGET_DRAW_DEBUG_RECT
      bool m_is_debug_rect = false;
      Widget* m_debug_rect{};
#endif
  };

} // namespace zincgui
