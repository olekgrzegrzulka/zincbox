#pragma once
#include <glm/ext/vector_float2.hpp>
#include "../types.hpp"
#include "button.hpp"
#include "sprite.hpp"
#include "widget.hpp"

class UI;

enum class ScrollBarOrientation {
  HORIZONTAL,
  VERTICAL,
};

class ScrollBar : public Widget {
protected:
  Sprite& track;
  Sprite& thumb;
  bool is_dragged = false;
  std::function<void(i32)> lambda = nullptr;

  i32 content_size = 1000;
  i32 page_size = 200;
  i32 old_scroll_offset = 0;
  i32 scroll_offset = 0; // between 0 and (content_size - page_size)

  i32 drag_start_mouse_pos = 0;
  i32 drag_start_thumb_pos = 0;
  i32 drag_start_scroll_offset = 0;

  ScrollBarOrientation orientation{};

public:
  ScrollBar(UI& ui_, ScrollBarOrientation orientation_ = ScrollBarOrientation::VERTICAL) : Widget::Widget(ui_), track(add_child<Sprite>()), thumb(add_child<Sprite>()) {
    using enum ScrollBarOrientation;

    orientation = orientation_;

    set_size(10, 10);

    track.set_texture("scrollbar_track");
    track.set_nine_slice_margin(6.0f);
    if (orientation_ == HORIZONTAL) {
      track.set_height(10);
      track.set_parent_anchor(Anchor::CENTER_LEFT);
      track.set_anchor(Anchor::CENTER_LEFT);
    } else {
      track.set_width(10);
      track.set_parent_anchor(Anchor::TOP_CENTER);
      track.set_anchor(Anchor::TOP_CENTER);
    }

    thumb.set_nine_slice_margin(6.0f);
    thumb.set_width(10);
    thumb.set_height(10);
    if (orientation_ == HORIZONTAL) {
      thumb.set_parent_anchor(Anchor::CENTER_LEFT);
      thumb.set_anchor(Anchor::CENTER_LEFT);
    } else {
      thumb.set_parent_anchor(Anchor::TOP_CENTER);
      thumb.set_anchor(Anchor::TOP_CENTER);
    }
  }

  void on_scroll_offset_changed(std::function<void(i32)> lambda_) {
    lambda = lambda_;
  }

  void update() override;

  void draw() override {
  }

  void scroll(float force);

  void handle_event(Input::InputEventMouseButton&) override;
  void handle_event(Input::InputEventMouseMove&) override;
  void handle_event(Input::InputEventMouseScroll&) override;
  void handle_event(Input::InputEventKey&) override;
  void handle_event(Input::InputEventMouseEntered&) override;
  void handle_event(Input::InputEventMouseLeft&) override;
  void handle_event(Input::InputEventCloseWindow&) override;

  WIDGET_DEF_GETTER(content_size)
  WIDGET_DEF_GETTER(page_size)
  WIDGET_DEF_GETTER(scroll_offset)

  WIDGET_DEF_SETTER_DIRTY(content_size)
  WIDGET_DEF_SETTER_DIRTY(page_size)
  WIDGET_DEF_SETTER_DIRTY(scroll_offset)
};
