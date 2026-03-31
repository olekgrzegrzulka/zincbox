#include <algorithm>
#include "common/input.hpp"
#include "common/types.hpp"
#include "scrollbar.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "widget.hpp"

ScrollBar::ScrollBar(UI& ui_, SliderOrientation o) : Slider(ui_, o) {
  set_texture_track("scrollbar_track");
  set_texture_thumb_pressed("scrollbar_thumb_pressed");
  set_texture_thumb_hovered("scrollbar_thumb_hovered");
  set_texture_thumb_idle("scrollbar_thumb_idle");
  set_texture_track("scrollbar_track");
}

void ScrollBar::update() {
  using enum SliderOrientation;
  min_value = 0;
  max_value = std::max(0, content_size - page_size);
  i32 track_length = (orientation == HORIZONTAL) ? width : height;
  thumb_length = std::clamp((page_size / (float)content_size) * track_length, 40.0f, (float)track_length);

  value = std::clamp(value, min_value, max_value);

  Slider::update();
}

void ScrollBar::draw() {
  Slider::draw();
}

void ScrollBar::scroll(float force) {
  value = std::clamp(value - force * 40, min_value, max_value);
}

void ScrollBar::handle_event(Input::InputEventMouseScroll& ev) {
  if (!is_dragged && is_mouse_hovering()) {
    scroll(ev.offset.y);
    ev.handled = true;
  }
}
