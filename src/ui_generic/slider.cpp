#include <algorithm>
#include "common/input.hpp"
#include "common/types.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "ui.hpp"
#include "widget.hpp"

Slider::Slider(UI& ui_, SliderOrientation orientation_) : Widget::Widget(ui_), track(add_child<Sprite>()), thumb(track.add_child<Sprite>()) {
  orientation = orientation_;

  set_size(100, 10);
  track.set_nine_slice_margin(6.0f);
  thumb.set_nine_slice_margin(6.0f);

  set_texture_track("slider_track");
  set_texture_thumb_pressed("slider_thumb_pressed");
  set_texture_thumb_hovered("slider_thumb_hovered");
  set_texture_thumb_idle("slider_thumb_idle");
  set_texture_track("slider_track");
}

void Slider::update() {
  using enum Anchor;
  using enum Input::MouseButton;
  using enum SliderOrientation;

  track.set_uv_start(uv_start_track);
  track.set_uv_end(uv_end_track);

  i32 track_length = (orientation == HORIZONTAL) ? width : height;
  if (orientation == HORIZONTAL) {
    track.set_width(track_length);
    track.set_height(track_thickness);
    track.set_parent_anchor(CENTER);
    track.set_anchor(CENTER);

    thumb.set_width(thumb_length);
    thumb.set_height(thumb_thickness);
    thumb.set_parent_anchor(CENTER_LEFT);
    thumb.set_anchor(CENTER_LEFT);

  } else {
    track.set_width(track_thickness);
    track.set_height(track_length);
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

    float drag = drag_px / (float)(track_length - thumb_length) * (max_value - min_value);
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
    thumb.set_uv_start(uv_start_thumb_pressed);
    thumb.set_uv_end(uv_end_thumb_pressed);
  } else if (mouse_on_thumb && !lmb_pressed) {
    thumb.set_uv_start(uv_start_thumb_hovered);
    thumb.set_uv_end(uv_end_thumb_hovered);
  } else {
    thumb.set_uv_start(uv_start_thumb_idle);
    thumb.set_uv_end(uv_end_thumb_idle);
  }

  float thumb_old_pos = (orientation == HORIZONTAL) ? thumb.get_x() : thumb.get_y();
  float thumb_pos_target = 0.0f;
  if (thumb_constraint == ThumbConstraint::FULL_RANGE) {
    thumb_pos_target = ((value - min_value) / (float)(max_value - min_value)) * (track_length);
    thumb_pos_target -= thumb_length * 0.5f;
  } else {
    thumb_pos_target = ((value - min_value) / (float)(max_value - min_value)) * (track_length - thumb_length);
  }
  double t = std::clamp(std::abs(thumb_old_pos - thumb_pos_target) * 0.004, 0.6, 0.95);
  float thumb_pos_lerped = std::lerp(thumb_old_pos, thumb_pos_target, t);
  if (orientation == HORIZONTAL) {
    thumb.set_x(thumb_pos_lerped);
  } else if (orientation == VERTICAL) {
    thumb.set_y(thumb_pos_lerped);
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

void Slider::set_texture_thumb_pressed(const std::string& id) {
  auto t_ = ui.get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != uv_start_thumb_pressed) {
      uv_start_thumb_pressed = (*t_).get().start;
      uv_end_thumb_pressed = (*t_).get().end;
      mark_dirty();
    }
  }
}
void Slider::set_texture_thumb_hovered(const std::string& id) {
  auto t_ = ui.get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != uv_start_thumb_hovered) {
      uv_start_thumb_hovered = (*t_).get().start;
      uv_end_thumb_hovered = (*t_).get().end;
      mark_dirty();
    }
  }
}
void Slider::set_texture_thumb_idle(const std::string& id) {
  auto t_ = ui.get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != uv_start_thumb_idle) {
      uv_start_thumb_idle = (*t_).get().start;
      uv_end_thumb_idle = (*t_).get().end;
      mark_dirty();
    }
  }
}
void Slider::set_texture_track(const std::string& id) {
  auto t_ = ui.get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != uv_start_track) {
      uv_start_track = (*t_).get().start;
      uv_end_track = (*t_).get().end;
      mark_dirty();
    }
  }
}
