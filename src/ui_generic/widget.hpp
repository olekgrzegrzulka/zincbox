#pragma once
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <glm/vec2.hpp>
#include "common/debug.hpp"
#include "common/types.hpp"

class UI;

#define WIDGET_DEF_SETTER_DIRTY(field)         \
  void set_##field(decltype(field) field##_) { \
    if (field == field##_) { return; }         \
    field = field##_;                          \
    dirty = true;                              \
  }

#define WIDGET_DEF_SETTER(field)               \
  void set_##field(decltype(field) field##_) { \
    if (field == field##_) { return; }         \
    field = field##_;                          \
  }

#define WIDGET_DEF_GETTER(field) \
  decltype(field) get_##field() const { return field; }

enum class LayoutDirection {
  LEFT_TO_RIGHT,
  RIGHT_TO_LEFT,
  TOP_TO_BOTTOM,
  BOTTOM_TO_TOP,
};

enum class Anchor {
  TOP_LEFT,
  TOP_CENTER,
  TOP_RIGHT,
  CENTER_LEFT,
  CENTER_CENTER,
  CENTER_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_CENTER,
  BOTTOM_RIGHT,

  LEFT = CENTER_LEFT,
  RIGHT = CENTER_RIGHT,
  TOP = TOP_CENTER,
  BOTTOM = BOTTOM_CENTER,
  CENTER = CENTER_CENTER,
};

struct Layout {
    bool enabled = false;
    bool fit_to_contents = false;
    bool expand_children = false;
    bool fill = false;
    i32 margin = 0;
    i32 spacing = 0;
    LayoutDirection direction = LayoutDirection::LEFT_TO_RIGHT;

    auto operator<=>(const Layout&) const = default;
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

// clang-format off
inline vec2f anchor_to_uv(Anchor anchor) {
  using enum Anchor;
  switch (anchor) {
  case TOP_LEFT:      {return {0.0, 0.0};}
  case TOP_CENTER:    {return {0.5, 0.0};}
  case TOP_RIGHT:     {return {1.0, 0.0};}
  case CENTER_LEFT:   {return {0.0, 0.5};}
  case CENTER_CENTER: {return {0.5, 0.5};}
  case CENTER_RIGHT:  {return {1.0, 0.5};}
  case BOTTOM_LEFT:   {return {0.0, 1.0};}
  case BOTTOM_CENTER: {return {0.5, 1.0};}
  case BOTTOM_RIGHT:  {return {1.0, 1.0};}
  }
  ensure(false);
  return {0.0, 0.0};
}
// clang-format on

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
  protected:
    UI& ui;
    std::string name;
    i32 x = 0;
    i32 y = 0;

    i32 width = 64;
    i32 height = 64;

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
    bool clip_children = false;
    bool ignore_parents_layout = false;
    bool is_drawn_on_top = false;
    float weight = 1.0; // FIXME: weight does nothing at the moment
    bool draw_behind_parent = false;

    Widget* parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;
    bool update_children_first = false;

    Layout layout;

  private:
    i32 window_width = 0;
    i32 window_height = 0;
#ifdef WIDGET_DRAW_DEBUG_RECT
    bool is_debug_rect = false;
    Widget* debug_rect{};
#endif

  public:
    Widget(UI& ui_) : ui{ui_} {}
    Widget(UI& ui_, i32 width_, i32 height_) : ui{ui_}, width{width_}, height{height_} {}
    virtual ~Widget() {};

    virtual void input();
    virtual void update();
    virtual void draw();

    // Returns widget's scene position relative to some anchor
    // (i.e. using Anchor::CENTER_CENTER will yield the center position of the widget)
    vec2i get_position(Anchor relative_to = Anchor::TOP_LEFT) const;

    WIDGET_DEF_GETTER(name);
    WIDGET_DEF_SETTER(name);
    WIDGET_DEF_SETTER(marked_for_deletion)
    WIDGET_DEF_SETTER_DIRTY(is_updated)
    WIDGET_DEF_SETTER_DIRTY(x)
    WIDGET_DEF_SETTER_DIRTY(y)
    void set_width(i32);
    void set_height(i32);
    WIDGET_DEF_SETTER_DIRTY(min_width)
    WIDGET_DEF_SETTER_DIRTY(max_width)
    WIDGET_DEF_SETTER_DIRTY(min_height)
    WIDGET_DEF_SETTER_DIRTY(max_height)
    WIDGET_DEF_SETTER_DIRTY(anchor)
    WIDGET_DEF_SETTER_DIRTY(parent_anchor)
    WIDGET_DEF_SETTER_DIRTY(window_width)
    WIDGET_DEF_SETTER_DIRTY(window_height)
    WIDGET_DEF_SETTER_DIRTY(update_children_first)
    WIDGET_DEF_SETTER_DIRTY(is_self_drawn)
    WIDGET_DEF_SETTER_DIRTY(clip_children)

    void set_is_drawn(bool to) {
      if (is_drawn == to) { return; }
      is_drawn = to;
      mark_dirty();
      if (parent) {
        parent->mark_dirty();
      }
    }

    void set_ignore_parents_layout(bool to) {
      if (ignore_parents_layout == to) { return; }
      ignore_parents_layout = to;
      mark_dirty();
      if (parent) {
        parent->mark_dirty();
      }
    }

    WIDGET_DEF_SETTER_DIRTY(is_drawn_on_top)
    WIDGET_DEF_SETTER_DIRTY(weight)
    WIDGET_DEF_SETTER_DIRTY(draw_behind_parent)

    WIDGET_DEF_GETTER(dirty)
    WIDGET_DEF_GETTER(marked_for_deletion)
    WIDGET_DEF_GETTER(is_updated)
    WIDGET_DEF_GETTER(x)
    WIDGET_DEF_GETTER(y)
    i32 get_width() const {
      i32 lo = min_width > 0 ? min_width : std::numeric_limits<i32>::min();
      i32 hi = max_width > 0 ? max_width : std::numeric_limits<i32>::max();
      return std::clamp(width, lo, hi);
    }
    i32 get_height() const {
      i32 lo = min_height > 0 ? min_height : std::numeric_limits<i32>::min();
      i32 hi = max_height > 0 ? max_height : std::numeric_limits<i32>::max();
      return std::clamp(height, lo, hi);
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
    WIDGET_DEF_GETTER(clip_children)
    WIDGET_DEF_GETTER(ignore_parents_layout)
    WIDGET_DEF_GETTER(is_drawn_on_top)
    WIDGET_DEF_GETTER(weight)
    WIDGET_DEF_GETTER(draw_behind_parent)
    WIDGET_DEF_GETTER(parent)

    Layout& get_layout() {
      mark_dirty();
      return layout;
    }

    void set_layout(std::string_view def);

    void set_pos(i32 x, i32 y);
    void set_pos(vec2i);

    void set_size(i32 w, i32 h);
    void set_size(vec2i);

    void mark_dirty() {
      dirty = true;
    }

    auto& get_children() { return children; }

  public:
    template <class T, class... Args>
    T& add_child(Args&&... args) {
      static_assert(std::is_base_of_v<Widget, T>);
      children.emplace_back(std::make_unique<T>(ui, std::forward<Args>(args)...));
      T& widget = static_cast<T&>(*children.back().get());
      widget.parent = this;
      return widget;
    }

    bool is_mouse_hovering() const;
    bool is_mouse_hovering(vec2i) const;

    virtual void event(Input::InputEventMouseButton&) {}
    virtual void event(Input::InputEventMouseMove&) {}
    virtual void event(Input::InputEventMouseScroll&) {}
    virtual void event(Input::InputEventKey&) {}
    virtual void event(Input::InputEventMouseEntered&) {}
    virtual void event(Input::InputEventMouseLeft&) {}
    virtual void event(Input::InputEventCloseWindow&) {}
};
