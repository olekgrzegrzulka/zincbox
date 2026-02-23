#include "slider.hpp"
#include <algorithm>
#include <cmath>
#include "../input.hpp"
#include "../types.hpp"
#include "sprite.hpp"

void Slider::update() {
  if (dirty) {

    // FIXME: this can cause a 1 frame delay when children are updated BEFORE parent
    for (auto&& c : children) {
      c->mark_dirty();
    }

    dirty = false;
  }

  static constexpr i32 x_margin = 10;
  bool mouse_on_widget = is_mouse_hovering();
  bool lmb_pressed = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb_just_released = Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT);

  if (lmb_just_pressed && mouse_on_widget) {
    is_dragged = true;
  }
  if (lmb_just_released) {
    is_dragged = false;
  }

  i32 old_value = value;
  if (is_dragged && lmb_pressed) {
    i32 x_rel = std::clamp(Input::get_mouse_x() - get_position(Anchor::TOP_LEFT).x, 0, width);
    value = std::round(x_rel / (float)width * (max_value - min_value) + min_value);
  }

  if (old_value != value && lambda) {
    lambda(value);
  }

  track.set_width(width);

  const i32 m = 6;
  float thumb_x = (value - min_value) / (float)(max_value - min_value);
  thumb_x = m + thumb_x * (width - 2 * m);
  thumb.set_x(std::lerp(thumb.get_x(), thumb_x, 0.9f));

  bool mouse_on_thumb = thumb.is_mouse_hovering();
  if (is_dragged) {
    thumb.set_texture("slider_thumb_pressed");
  } else if (mouse_on_thumb && !lmb_pressed) {
    thumb.set_texture("slider_thumb_hovered");
  } else {
    thumb.set_texture("slider_thumb_idle");
  }
}
