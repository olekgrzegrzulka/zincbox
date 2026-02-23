#include <algorithm>
#include "../input.hpp"
#include "../types.hpp"
#include "scrollbar.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "widget.hpp"

ScrollBar::ScrollBar(UI& ui_, SliderOrientation o) : Slider(ui_, o) {
}

void ScrollBar::update() {
  using enum SliderOrientation;
  min_value = 0;
  max_value = std::max(0, content_size - page_size);
  thumb_length = std::max(40.0f, (page_size / (float)max_value) * track_length);

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
