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

#define WIDGET_DEF_SETTER_DIRTY(field)                                                                                 \
  void set_##field(decltype(field) field##_) {                                                                         \
    if ((field) == field##_) { return; }                                                                               \
    (field) = field##_;                                                                                                \
    mark_dirty();                                                                                                      \
  }

#define WIDGET_DEF_SETTER(field)                                                                                       \
  void set_##field(decltype(field) field##_) {                                                                         \
    if ((field) == field##_) { return; }                                                                               \
    (field) = field##_;                                                                                                \
  }

#define WIDGET_DEF_GETTER(field)                                                                                       \
  decltype(field) get_##field() const { return field; }

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

    protected:
      Root& ui;
      std::vector<std::unique_ptr<Widget>> children_to_add;
      std::string name;
      i32 x = 0;
      i32 y = 0;
      bool hover_test_parent = true;
      i32 width = default_settings::widget_size.x;
      i32 height = default_settings::widget_size.y;

      i32 min_width = 0;
      i32 max_width = 0;
      i32 min_height = 0;
      i32 max_height = 0;

      Anchor anchor = Anchor::TOP_LEFT;
      Anchor parent_anchor = Anchor::TOP_LEFT;

      bool dirty = true;
      bool marked_for_deletion = false;
      bool is_updated = true;
      bool is_drawn = true;
      bool is_self_drawn = true;
      bool clip = false;
      bool clip_children = false;
      bool is_focusable = false;
      bool ignore_parents_layout = false;
      bool is_drawn_on_top = false;
      float weight = default_settings::widget_weight; // FIXME: weight does nothing at the moment
      bool draw_behind_parent = false;
      HitTestBehavior hit_test_behavior = HitTestBehavior::PASS_THROUGH;

      bool hovered_ = false;
      bool visible_ = false;
      bool updated_ = false;
      u64 last_update_frame_ = 0;
      bool deleted_ = false;
      i32 always_on_top_layer_ = 0;
      std::optional<rect2i> scissor_;
      rect2i rect_;
      rect2i hit_test_rect_;
      // FIXME add property for hit test rect and allow expanding it (currently we use rect_)
      vec2i global_position_{};

      Widget* parent = nullptr;
      std::vector<std::unique_ptr<Widget>> children;
      bool update_children_first = false;

      Layout layout;

      bool is_layout_dirty() const { return m_layout_dirty; }

    private:
      bool m_layout_dirty = false;
      i32 window_width = 0;
      i32 window_height = 0;
#ifdef WIDGET_DRAW_DEBUG_RECT
      bool is_debug_rect = false;
      Widget* debug_rect{};
#endif

    public:
      Widget(Root& ui_);
      Widget(Root& ui_, i32 width_, i32 height_);
      virtual ~Widget() {};

      virtual void input();
      virtual void update();
      virtual void on_focused() {}
      virtual void on_unfocused() {}

      // Returns widget's scene position relative to some anchor
      // (i.e. using Anchor::CENTER_CENTER will yield the center position of the widget)
      [[nodiscard]] ZINCBOX_INLINE vec2i get_position(Anchor relative_to = Anchor::TOP_LEFT) const noexcept {

        static constexpr std::pair<u8, u8> ANCHOR_SCALES[9] = {{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1},
                                                               {2, 1}, {0, 2}, {1, 2}, {2, 2}};

        const auto [sx, sy] = ANCHOR_SCALES[static_cast<u8>(relative_to)];
        return {global_position_.x + ((width * sx) >> 1), global_position_.y + ((height * sy) >> 1)};
      }

      WIDGET_DEF_GETTER(name);
      void set_name(std::string name_);
      void set_marked_for_deletion(bool value);
      void set_is_updated(bool value);
      void set_x(i32, bool ignore_constraints = false);
      void set_y(i32, bool ignore_constraints = false);
      void set_width(i32, bool ignore_constraints = false);
      void set_height(i32, bool ignore_constraints = false);
      void set_anchor(Anchor, bool ignore_constraints = false);
      void set_parent_anchor(Anchor, bool ignore_constraints = false);
      void set_min_width(i32);
      void set_max_width(i32);
      void set_min_height(i32);
      void set_max_height(i32);
      WIDGET_DEF_SETTER_DIRTY(window_width)
      WIDGET_DEF_SETTER_DIRTY(window_height)
      WIDGET_DEF_SETTER_DIRTY(is_focusable)
      WIDGET_DEF_SETTER_DIRTY(hit_test_behavior)
      WIDGET_DEF_GETTER(hit_test_behavior)
      void set_update_children_first(bool);
      void set_is_self_drawn(bool);
      void set_clip(bool);
      void set_clip_children(bool);

      void set_fixed_width(i32 value) {
        set_min_width(value);
        set_max_width(value);
      }

      void set_fixed_height(i32 value) {
        set_min_height(value);
        set_max_height(value);
      }

      void set_is_drawn(bool);

      void set_ignore_parents_layout(bool to) {
        if (ignore_parents_layout == to) { return; }
        ignore_parents_layout = to;
        if (parent && parent->layout.enabled) { parent->mark_layout_dirty(); }
        mark_dirty();
      }

      void set_is_drawn_on_top(bool);
      void set_weight(float);
      void set_draw_behind_parent(bool);

      WIDGET_DEF_GETTER(marked_for_deletion)
      WIDGET_DEF_GETTER(is_updated)
      WIDGET_DEF_GETTER(x)
      WIDGET_DEF_GETTER(y)
      bool is_focused() const;
      bool is_focus_manager() const;
      WIDGET_DEF_GETTER(is_focusable)
      i32 get_width() const {
        i32 lo = min_width > 0 ? min_width : std::numeric_limits<i32>::min();
        i32 hi = max_width > 0 ? max_width : std::numeric_limits<i32>::max();
        if (lo > hi) { std::swap(lo, hi); }
        return std::clamp(width, lo, hi);
      }
      i32 get_height() const {
        i32 lo = min_height > 0 ? min_height : std::numeric_limits<i32>::min();
        i32 hi = max_height > 0 ? max_height : std::numeric_limits<i32>::max();
        if (lo > hi) { std::swap(lo, hi); }
        return std::clamp(height, lo, hi);
      }
      vec2i get_size() const { return vec2i(width, height); }
      vec2i get_min_size() const { return vec2i(min_width, min_height); }
      vec2i get_max_size() const { return vec2i(max_width, max_height); }
      void set_fixed_size(vec2i s) {
        set_fixed_width(s.x);
        set_fixed_height(s.y);
      }
      void set_fixed_size(i32 w, i32 h) {
        set_fixed_width(w);
        set_fixed_height(h);
      }
      WIDGET_DEF_GETTER(min_width)
      WIDGET_DEF_GETTER(max_width)
      WIDGET_DEF_GETTER(min_height)
      WIDGET_DEF_GETTER(max_height)
      WIDGET_DEF_GETTER(anchor)
      WIDGET_DEF_GETTER(parent_anchor)
      WIDGET_DEF_GETTER(update_children_first)
      WIDGET_DEF_GETTER(is_drawn)
      WIDGET_DEF_GETTER(is_self_drawn)
      WIDGET_DEF_GETTER(clip)
      WIDGET_DEF_GETTER(clip_children)
      WIDGET_DEF_GETTER(ignore_parents_layout)
      WIDGET_DEF_GETTER(is_drawn_on_top)
      WIDGET_DEF_GETTER(weight)
      WIDGET_DEF_GETTER(draw_behind_parent)
      WIDGET_DEF_GETTER(parent)
      WIDGET_DEF_GETTER(hover_test_parent)

      rect2i get_rect() const; // FIXME: inverted y-axis

      Layout& get_layout() {
        // FIXME: mark layout as dirty if changed
        return layout;
      }

      bool has_layout() const { return layout.enabled; }

      void set_layout(std::string_view def);

      void set_pos(i32 x, i32 y);
      void set_pos(vec2i);

      void set_size(i32 w, i32 h);
      void set_size(vec2i);

      void mark_layout_dirty();
      void mark_dirty();

      auto& get_children() { return children; }

    public:
      template <class T, class... Args> T& add_child(Args&&... args) {
        static_assert(std::is_base_of_v<Widget, T>);
        children_to_add.emplace_back(std::make_unique<T>(ui, std::forward<Args>(args)...));
        T& widget = static_cast<T&>(*children_to_add.back().get());
        widget.parent = this;
        mark_dirty();
        return widget;
      }

      bool is_mouse_hovering() const;
      bool is_mouse_hovering(vec2i) const;

      virtual std::span<const Vertex> get_vertices() {
        static const std::vector<Vertex> EMPTY = {};
        return EMPTY;
      }

      virtual void event(Input::InputEventMouseButton&) {}
      virtual void event(Input::InputEventMouseMove&) {}
      virtual void event(Input::InputEventMouseScroll&) {}
      virtual void event(Input::InputEventKey&) {}
      virtual void event(Input::InputEventMouseEntered&) {}
      virtual void event(Input::InputEventMouseLeft&) {}
      virtual void event(Input::InputEventCloseWindow&) {}
  };

} // namespace zincgui
