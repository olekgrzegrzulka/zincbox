#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "scrollbar.hpp"
#include "slider.hpp"
#include "ui/zincgui/default_settings.hpp"

zincgui::ScrollBar::ScrollBar(Root& ui_, SliderOrientation o) : Slider(ui_, o) {
  set_orientation(SliderOrientation::VERTICAL);
  set_size(zincgui::default_settings::scrollbar_size);
  min_width = 0;
  max_width = 0;
  min_height = 0;
  max_height = 0;
  sensitivity = zincgui::default_settings::scrollbar_sensitivity;
  if (default_settings::scrollbar_fixed_width) { set_fixed_width(width); }
  set_thumb_thickness(width);
  set_track_thickness(width);
  set_texture_track_inactive(default_settings::scrollbar_texture_track_inactive);
  set_texture_track_active(default_settings::scrollbar_texture_track_active);
  set_texture_thumb_pressed(default_settings::scrollbar_texture_thumb_pressed);
  set_texture_thumb_hovered(default_settings::scrollbar_texture_thumb_hovered);
  set_texture_thumb_idle(default_settings::scrollbar_texture_thumb_idle);
  set_is_focusable(true);
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
  float min_thumb_length =
    std::min(track_length * default_settings::scrollbar_min_thumb_ratio, default_settings::scrollbar_min_thumb_max_px);

  float lo = std::min(min_thumb_length, (float)track_length);
  float hi = std::max(min_thumb_length, (float)track_length);
  thumb_length = std::clamp((page_size / (float)content_size) * track_length, lo, hi);
  set_value(value);
}

void zincgui::ScrollBar::scroll(float force) { set_value(value - (force * sensitivity)); }
