#include <algorithm>
#include "common/input.hpp"
#include "common/types.hpp"
#include "scrollbar.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "widget.hpp"

ScrollBar::ScrollBar(UI& ui_, SliderOrientation o) : Slider(ui_, o) {
  set_texture_track_inactive("scrollbar_track_inactive");
  set_texture_track_active("scrollbar_track_active");
  set_texture_thumb_pressed("scrollbar_thumb_pressed");
  set_texture_thumb_hovered("scrollbar_thumb_hovered");
  set_texture_thumb_idle("scrollbar_thumb_idle");
}

void ScrollBar::update() {
  recalculate_values();
  set_is_drawn(content_size > page_size);

  Slider::update();
}

void ScrollBar::recalculate_values() {
  using enum SliderOrientation;
  min_value = 0;
  max_value = std::max(0, content_size - page_size);
  i32 track_length = (orientation == HORIZONTAL) ? width : height;
  float min_thumb_length = std::min(track_length * 0.2f, 40.0f);
  thumb_length = std::clamp((page_size / (float)content_size) * track_length, min_thumb_length, (float)track_length);
  value = std::clamp(value, min_value, max_value);
}

void ScrollBar::draw() { Slider::draw(); }

void ScrollBar::scroll(float force) { value = std::clamp(value - force * sensitivity, min_value, max_value); }

void ScrollBar::event(Input::InputEventMouseScroll& ev) {
  if (is_mouse_hovering()) {
    if (!is_dragged) { scroll(ev.offset.y); }
    ev.handled = true;
  }
}

void ScrollBar::event(Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering()) { ev.handled = true; }
}
