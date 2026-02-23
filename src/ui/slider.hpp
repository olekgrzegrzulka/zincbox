#pragma once
#include <glm/ext/vector_float2.hpp>
#include "../types.hpp"
#include "button.hpp"
#include "sprite.hpp"
#include "widget.hpp"

class UI;

class Slider : public Widget {
protected:
  Sprite& track;
  Sprite& thumb;
  bool is_dragged = false;
  i32 value = 0;
  i32 min_value = -10;
  i32 max_value = 10;
  std::function<void(i32)> lambda = nullptr;

public:
  Slider(UI& ui_) : Widget::Widget(ui_), track(add_child<Sprite>()), thumb(add_child<Sprite>()) {
    set_size(64, 24);

    track.set_texture("slider_track");
    track.set_nine_slice_margin(6);
    track.set_height(12);
    track.set_parent_anchor(Anchor::CENTER_LEFT);
    track.set_anchor(Anchor::CENTER_LEFT);

    thumb.set_width(12);
    thumb.set_height(12);
    thumb.set_parent_anchor(Anchor::CENTER_LEFT);
    thumb.set_anchor(Anchor::CENTER_CENTER);
  }

  void on_value_changed(std::function<void(i32)> lambda_) {
    lambda = lambda_;
  }

  void update() override;

  void draw() override {
  }

  WIDGET_DEF_SETTER_DIRTY(value)
  WIDGET_DEF_SETTER_DIRTY(min_value)
  WIDGET_DEF_SETTER_DIRTY(max_value)
};