#include "scrollbar.hpp"
#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "slider.hpp"
#include "ui/zincgui/default_settings.hpp"

zincgui::ScrollBar::ScrollBar(Root& ui_, SliderOrientation o) : Slider(ui_, o) {
  orientation(SliderOrientation::VERTICAL)
    .size(zincgui::default_settings::scrollbar_size)
    .min_width(0)
    .max_width(0)
    .min_height(0)
    .max_height(0)
    .sensitivity(zincgui::default_settings::scrollbar_sensitivity);
  if (default_settings::scrollbar_fixed_width) { fixed_width(width()); }
  thumb_thickness(width())
    .track_thickness(width())
    .texture_track_inactive(default_settings::scrollbar_texture_track_inactive)
    .texture_track_active(default_settings::scrollbar_texture_track_active)
    .texture_thumb_pressed(default_settings::scrollbar_texture_thumb_pressed)
    .texture_thumb_hovered(default_settings::scrollbar_texture_thumb_hovered)
    .texture_thumb_idle(default_settings::scrollbar_texture_thumb_idle)
    .is_focusable(true);
}

void zincgui::ScrollBar::update() {
  recalculate_values();
  is_drawn(m_content_size > m_page_size);

  Slider::update();
}

void zincgui::ScrollBar::recalculate_values() {
  using enum SliderOrientation;
  min_value(0.0f);
  max_value(static_cast<float>(std::max(0, m_content_size - m_page_size)));
  i32 track_length = (orientation() == HORIZONTAL) ? width() : height();
  float min_thumb_length =
    std::min(track_length * default_settings::scrollbar_min_thumb_ratio, default_settings::scrollbar_min_thumb_max_px);

  float lo = std::min(min_thumb_length, static_cast<float>(track_length));
  float hi = std::max(min_thumb_length, static_cast<float>(track_length));
  thumb_length(std::clamp((m_page_size / static_cast<float>(m_content_size)) * track_length, lo, hi));
  value(value());
}

void zincgui::ScrollBar::scroll(float force) { value(value() - (force * sensitivity())); }
