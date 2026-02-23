#include <algorithm>
#include "../input.hpp"
#include "../types.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "widget.hpp"

Slider::Slider(UI& ui_, SliderOrientation orientation_) : Widget::Widget(ui_), track(add_child<Sprite>()), thumb(track.add_child<Sprite>()) {
  orientation = orientation_;

  set_size(100, 10);
  track.set_texture("scrollbar_track");
  track.set_nine_slice_margin(6.0f);
  thumb.set_nine_slice_margin(6.0f);
}

void Slider::update() {
  using enum Anchor;
  using enum Input::MouseButton;
  using enum SliderOrientation;

  i32 actual_track_length = 0;
  if (orientation == HORIZONTAL) {
    actual_track_length = std::min(track_length, width);
    track.set_width(actual_track_length);
    track.set_height(track_thickness);
    track.set_parent_anchor(CENTER);
    track.set_anchor(CENTER);

    thumb.set_width(thumb_length);
    thumb.set_height(thumb_thickness);
    thumb.set_parent_anchor(CENTER_LEFT);
    thumb.set_anchor(CENTER_LEFT);

  } else {
    track.set_width(track_thickness);
    actual_track_length = std::min(track_length, height);
    track.set_height(actual_track_length);
    track.set_parent_anchor(CENTER);
    track.set_anchor(CENTER);

    thumb.set_width(thumb_thickness);
    thumb.set_height(thumb_length);
    thumb.set_parent_anchor(TOP_CENTER);
    thumb.set_anchor(TOP_CENTER);
  }

  bool lmb_pressed = Input::mouse_pressed(MOUSE_BUTTON_LEFT);
  bool lmb_just_pressed = Input::mouse_just_pressed(MOUSE_BUTTON_LEFT);
  bool lmb_just_released = Input::mouse_just_released(MOUSE_BUTTON_LEFT);
  bool mouse_on_thumb = thumb.is_mouse_hovering();
  bool mouse_on_track = track.is_mouse_hovering();

  if (lmb_just_pressed && !mouse_on_thumb && mouse_on_track) {
    if (orientation == HORIZONTAL) {
      value = (Input::get_mouse_x() - get_position().x) / (float)width;
    } else {
      value = (Input::get_mouse_y() - get_position().y) / (float)height;
    }
    value *= (max_value - min_value);
    value += min_value;
    value = std::clamp(value, min_value, max_value);
    mouse_on_thumb = true;
  }

  if (lmb_just_pressed && mouse_on_thumb) {
    drag_start_value = value;
    if (orientation == HORIZONTAL) {
      drag_start_mouse_pos = Input::get_mouse_x();
      drag_start_thumb_pos = thumb.get_x();
    } else {
      drag_start_mouse_pos = Input::get_mouse_y();
      drag_start_thumb_pos = thumb.get_y();
    }
    is_dragged = true;
  }

  if (is_dragged) {
    i32 drag_px = Input::get_mouse_x() - drag_start_mouse_pos;
    if (orientation == VERTICAL) { drag_px = Input::get_mouse_y() - drag_start_mouse_pos; }

    float drag = drag_px / (float)(actual_track_length - thumb_length) * (max_value - min_value);
    value = std::clamp(drag_start_value + drag, min_value, max_value);
  }

  bool drag_ended = false;
  if (lmb_just_released && is_dragged) {
    is_dragged = false;
    drag_ended = true;
  }

  if (drag_ended && lambda_drag_end) {
    lambda_drag_end(drag_start_value, value);
  }

  if (is_dragged) {
    thumb.set_texture("scrollbar_thumb_pressed", false);
  } else if (mouse_on_thumb && !lmb_pressed) {
    thumb.set_texture("scrollbar_thumb_hovered", false);
  } else {
    thumb.set_texture("scrollbar_thumb_idle", false);
  }

  float thumb_pos = ((value - min_value) / (float)(max_value - min_value)) * (actual_track_length - thumb_length);
  if (orientation == HORIZONTAL) {
    double t = std::clamp(std::abs(thumb.get_x() - thumb_pos) * 0.004, 0.6, 0.95);
    thumb.set_x(std::lerp(thumb.get_x(), thumb_pos, t));
  } else if (orientation == VERTICAL) {
    double t = std::clamp(std::abs(thumb.get_y() - thumb_pos) * 0.004, 0.6, 0.95);
    thumb.set_y(std::lerp(thumb.get_y(), thumb_pos, t));
  }

  if (old_value != value) {
    if (lambda_value_changed) {
      lambda_value_changed(old_value, value);
    }
    old_value = value;
  }

  Widget::update();
}
void Slider::handle_event(Input::InputEventMouseButton&) {
}
void Slider::handle_event(Input::InputEventMouseMove&) {
}
void Slider::handle_event(Input::InputEventMouseScroll&) {
}
void Slider::handle_event(Input::InputEventKey&) {
}
void Slider::handle_event(Input::InputEventMouseEntered&) {
}
void Slider::handle_event(Input::InputEventMouseLeft&) {
}
void Slider::handle_event(Input::InputEventCloseWindow&) {
}
