#pragma once
#include "input.hpp"
#include "player.hpp"
#include "ui/sprite.hpp"
#include "ui/widget.hpp"

class UI;

class SeekBar : public Widget {
public:
  SeekBar(UI& ui_) : Widget(ui_), bg(add_child<Sprite>("seekbar_bg")), progress_bar(add_child<Sprite>("seekbar_progress")), thumb(add_child<Sprite>("seekbar_thumb")) {
    bg.set_parent_anchor(Anchor::LEFT);
    bg.set_anchor(Anchor::LEFT);
    bg.set_texture("seekbar_bg", false);
    bg.set_nine_slice_margin(6.0f);

    progress_bar.set_parent_anchor(Anchor::LEFT);
    progress_bar.set_anchor(Anchor::LEFT);
    progress_bar.set_nine_slice_margin(6.0f);

    thumb.set_parent_anchor(Anchor::LEFT);
    thumb.set_anchor(Anchor::CENTER);
    thumb.set_nine_slice_margin(6.0f);
    thumb.set_size(14, 14);
  }

  void update() override {
    if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
      is_thumb_dragged = false;
    }

    set_current_time_ms(player::get_current_time_ms());
    set_total_duration_ms(player::get_total_duration_ms());

    bg.set_width(width);
    bg.set_height(12);
    progress_bar.set_width(width);
    progress_bar.set_height(12);

    static i32 t = 0;
    t += 1;

    if (total_duration_ms > 0) {
      progress_bar.set_width(current_time_ms / (double)total_duration_ms * width);
    } else {
      progress_bar.set_width(0);
    }

    thumb.set_x(progress_bar.get_width());
    Widget::update();
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    if (is_mouse_hovering()) {
      ev.handled = true;
    }

    if (thumb.is_mouse_hovering()) {
      ev.handled = true;
      if (ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
        if (ev.action == Input::MouseAction::PRESS) {
          is_thumb_dragged = true;
        }
      }
    }
  }

protected:
  Sprite& bg;
  Sprite& progress_bar;
  Sprite& thumb;
  bool is_thumb_dragged = false;

  i32 total_duration_ms = 0;
  i32 current_time_ms = 0;
  bool is_active = false;

public:
  WIDGET_DEF_SETTER_DIRTY(total_duration_ms)
  WIDGET_DEF_SETTER_DIRTY(current_time_ms)
  WIDGET_DEF_SETTER_DIRTY(is_active)
};
