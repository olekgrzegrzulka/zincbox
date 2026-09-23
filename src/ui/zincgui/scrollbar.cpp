#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "scrollbar.hpp"
#include "slider.hpp"

zincgui::ScrollBar::ScrollBar(Root& ui_, SliderOrientation o) : Slider(ui_, o) {
  set_texture_track_inactive("scrollbar_track_inactive");
  set_texture_track_active("scrollbar_track_active");
  set_texture_thumb_pressed("scrollbar_thumb_pressed");
  set_texture_thumb_hovered("scrollbar_thumb_hovered");
  set_texture_thumb_idle("scrollbar_thumb_idle");
}

void zincgui::ScrollBar::update() {
  recalculate_values();
  set_is_drawn(content_size > page_size);

  Slider::update();
}

void zincgui::ScrollBar::recalculate_values() {
  using enum SliderOrientation;
  min_value = 0;
  max_value = std::max(0, content_size - page_size);
  i32 track_length = (orientation == HORIZONTAL) ? width : height;
  float min_thumb_length = std::min(track_length * 0.2f, 40.0f);

  float lo = std::min(min_thumb_length, (float)track_length);
  float hi = std::max(min_thumb_length, (float)track_length);
  thumb_length = std::clamp((page_size / (float)content_size) * track_length, lo, hi);
  set_value(value);
}

void zincgui::ScrollBar::draw() { Slider::draw(); }

void zincgui::ScrollBar::scroll(float force) { set_value(value - force * sensitivity); }
