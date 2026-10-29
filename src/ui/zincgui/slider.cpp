#include <algorithm>
#include <cmath>
#include <optional>
#include "common/types.hpp"
#include "slider.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/widget.hpp"

zincgui::Slider::Slider(Root& ui_, SliderOrientation orientation_)
  : Widget::Widget(ui_), m_track(add_child<Sprite>()), m_track_active(m_track.add_child<Sprite>()),
    m_thumb(m_track.add_child<Sprite>()) {
  m_orientation = orientation_;

  is_focusable(true)
    .hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH)
    .size(default_settings::slider_size);

  if (default_settings::slider_fixed_height) { fixed_height(height()); }

  texture_thumb_pressed(default_settings::slider_texture_thumb_pressed);
  texture_thumb_hovered(default_settings::slider_texture_thumb_hovered);
  texture_thumb_idle(default_settings::slider_texture_thumb_idle);
  texture_track_inactive(default_settings::slider_texture_track_inactive);
  texture_track_active(default_settings::slider_texture_track_active);
}

void zincgui::Slider::input() {
  using enum Anchor;
  using enum Input::MouseButton;
  using enum SliderOrientation;

  bool is_horiz = (m_orientation == HORIZONTAL || m_orientation == HORIZONTAL_INVERTED);
  bool is_invert = (m_orientation == HORIZONTAL_INVERTED || m_orientation == VERTICAL_INVERTED);

  m_track.nine_slice_margin(m_track_nine_slice_margin)
         .uv_start(m_uv_start_track_inactive)
         .uv_end(m_uv_end_track_inactive);

  m_track_active.nine_slice_margin(m_track_nine_slice_margin)
                .uv_start(m_uv_start_track_active)
                .uv_end(m_uv_end_track_active);

  m_thumb.nine_slice_margin(m_thumb_nine_slice_margin);

  i32 track_len = is_horiz ? width() : height();
  if (is_horiz) {
    m_track.width(track_len)
           .height(m_track_thickness)
           .parent_anchor(CENTER)
           .anchor(CENTER);

    m_track_active.height(m_track_thickness)
                  .parent_anchor(is_invert ? RIGHT : LEFT)
                  .anchor(is_invert ? RIGHT : LEFT);

    m_thumb.width(m_thumb_length)
           .height(m_thumb_thickness)
           .parent_anchor(CENTER_LEFT)
           .anchor(CENTER_LEFT);

  } else {
    m_track.width(m_track_thickness)
           .height(track_len)
           .parent_anchor(CENTER)
           .anchor(CENTER);

    m_track_active.width(m_track_thickness)
                  .parent_anchor(is_invert ? BOTTOM : TOP)
                  .anchor(is_invert ? BOTTOM : TOP);

    m_thumb.width(m_thumb_thickness)
           .height(m_thumb_length)
           .parent_anchor(TOP_CENTER)
           .anchor(TOP_CENTER);
  }

  bool lmb_pressed = false;
  bool mouse_on_thumb = false;

  if (!m_disabled) {
    lmb_pressed = Input::mouse_pressed(MOUSE_BUTTON_LEFT);
    bool lmb_just_pressed = Input::mouse_just_pressed(MOUSE_BUTTON_LEFT);
    bool lmb_just_released = Input::mouse_just_released(MOUSE_BUTTON_LEFT);
    bool mouse_on_track = false;
    // FIXME: drag_area_inflation not used
    mouse_on_thumb = m_thumb.is_mouse_hovering();
    mouse_on_track = m_track.is_mouse_hovering();

    if (lmb_just_pressed && !mouse_on_thumb && mouse_on_track) {
      float new_value = 0.0f;
      if (is_horiz && width() != 0) {
        new_value = (Input::get_mouse_x() - position().x) / (float)width();
      } else if (height() != 0) {
        new_value = (Input::get_mouse_y() - position().y) / (float)height();
      }
      if (is_invert) { new_value = 1.0f - new_value; }
      new_value *= (m_max_value - m_min_value);
      new_value += m_min_value;
      value(new_value);
      mouse_on_thumb = true;
    }

    if (lmb_just_pressed && mouse_on_thumb) {
      m_drag_start_value = m_value;
      if (is_horiz) {
        m_drag_start_mouse_pos = Input::get_mouse_x();
        m_drag_start_thumb_pos = m_thumb.x();
      } else {
        m_drag_start_mouse_pos = Input::get_mouse_y();
        m_drag_start_thumb_pos = m_thumb.y();
      }
      m_is_dragged = true;
    }

    if (m_is_dragged) {
      i32 drag_px =
        is_horiz ? (Input::get_mouse_x() - m_drag_start_mouse_pos) : (Input::get_mouse_y() - m_drag_start_mouse_pos);
      if (is_invert) { drag_px = -drag_px; }

      if ((track_len - m_thumb_length) != 0) {
        float drag = drag_px / (float)(track_len - m_thumb_length) * (m_max_value - m_min_value);
        value(m_drag_start_value + drag);
      }
    }

    bool drag_ended = false;
    if (lmb_just_released && m_is_dragged) {
      m_is_dragged = false;
      drag_ended = true;
    }

    if (drag_ended && m_on_drag_ended) { m_on_drag_ended(m_drag_start_value, m_value); }
  }

  if (m_is_dragged) {
    m_state = SliderState::PRESSED;
    if (m_state != m_state_old) {
      m_thumb.uv_start(m_uv_start_thumb_pressed).uv_end(m_uv_end_thumb_pressed);
      m_state_old = m_state;
    }
  } else if (mouse_on_thumb && !lmb_pressed) {
    m_state = SliderState::HOVERED;
    if (m_state != m_state_old) {
      m_thumb.uv_start(m_uv_start_thumb_hovered).uv_end(m_uv_end_thumb_hovered);
      m_state_old = m_state;
    }
  } else {
    m_state = SliderState::IDLE;
    if (m_state != m_state_old) {
      m_thumb.uv_start(m_uv_start_thumb_idle).uv_end(m_uv_end_thumb_idle);
      m_state_old = m_state;
    }
  }

  float norm_target = m_norm_pos_anim;
  if ((m_max_value - m_min_value) != 0.0f) {
    float norm = (m_value - m_min_value) / (m_max_value - m_min_value);
    if (is_invert) { norm = 1.0f - norm; }
    norm_target = norm;
  }
  double t =
    std::clamp(std::abs(m_norm_pos_anim - norm_target) * (track_len * default_settings::slider_anim_speed_factor),
               default_settings::slider_anim_lerp_min, default_settings::slider_anim_lerp_max);
  m_norm_pos_anim = std::lerp(m_norm_pos_anim, norm_target, static_cast<float>(t));

  float thumb_pos_lerped = 0.0f;
  if (m_thumb_constraint == ThumbConstraint::FULL_RANGE) {
    thumb_pos_lerped = (m_norm_pos_anim * track_len) - (m_thumb_length * 0.5f);
  } else {
    thumb_pos_lerped = m_norm_pos_anim * (track_len - m_thumb_length);
  }

  if (is_horiz) {
    m_thumb.x(thumb_pos_lerped);
    m_track_active.width(is_invert ? (track_len - (thumb_pos_lerped + m_thumb.width() * 0.5))
                                   : (thumb_pos_lerped + (m_thumb.width() * 0.5)));
  } else {
    m_thumb.y(thumb_pos_lerped);
    m_track_active.height(is_invert ? (track_len - (thumb_pos_lerped + m_thumb.height() * 0.5))
                                    : (thumb_pos_lerped + (m_thumb.height() * 0.5)));
  }

  Widget::input();
}

void zincgui::Slider::update() { Widget::update(); }

void zincgui::Slider::event(Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering()) { ev.handled = true; }
}

void zincgui::Slider::event(Input::InputEventMouseMove& ev) {
  if (is_mouse_hovering()) { ev.handled = true; }
}

void zincgui::Slider::event(Input::InputEventMouseScroll& ev) {
  if (is_mouse_hovering() && m_is_scrollable) {
    if (!m_is_dragged) {
      i32 sign = ev.offset.y / std::abs(ev.offset.y);
      value(m_value + ((float)sign * m_sensitivity));
      ev.handled = true;
    }
  }
}

void zincgui::Slider::event(Input::InputEventKey&) {}

void zincgui::Slider::skip_anim() {
  using enum SliderOrientation;
  bool is_horiz = (m_orientation == HORIZONTAL || m_orientation == HORIZONTAL_INVERTED);
  bool is_invert = (m_orientation == HORIZONTAL_INVERTED || m_orientation == VERTICAL_INVERTED);

  i32 track_len = is_horiz ? width() : height();
  if ((m_max_value - m_min_value) != 0.0f) {
    float norm = (m_value - m_min_value) / (m_max_value - m_min_value);
    if (is_invert) { norm = 1.0f - norm; }
    m_norm_pos_anim = norm;
  }

  float thumb_pos = 0.0f;
  if (m_thumb_constraint == ThumbConstraint::FULL_RANGE) {
    thumb_pos = (m_norm_pos_anim * track_len) - (m_thumb_length * 0.5f);
  } else {
    thumb_pos = m_norm_pos_anim * (track_len - m_thumb_length);
  }

  if (is_horiz) {
    m_thumb.x(thumb_pos);
    m_track_active.width(is_invert ? (track_len - (thumb_pos + m_thumb.width() * 0.5))
                                   : (thumb_pos + (m_thumb.width() * 0.5)));
  } else {
    m_thumb.y(thumb_pos);
    m_track_active.height(is_invert ? (track_len - (thumb_pos + m_thumb.height() * 0.5))
                                    : (thumb_pos + (m_thumb.height() * 0.5)));
  }
}

zincgui::Slider& zincgui::Slider::orientation(SliderOrientation o) {
  if (m_orientation == o) { return *this; }

  auto is_horiz = [](SliderOrientation orient) -> bool {
    return orient == SliderOrientation::HORIZONTAL || orient == SliderOrientation::HORIZONTAL_INVERTED;
  };

  if (is_horiz(m_orientation) != is_horiz(o)) {
    i32 cur_min_w = min_width();
    i32 cur_min_h = min_height();
    i32 cur_max_w = max_width();
    i32 cur_max_h = max_height();
    min_width(cur_min_h).min_height(cur_min_w)
      .max_width(cur_max_h).max_height(cur_max_w);
  }

  m_orientation = o;
  mark_dirty();
  return *this;
}

zincgui::Slider& zincgui::Slider::texture_thumb_pressed(const std::string& id) {
  auto t_ = ui().get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != m_uv_start_thumb_pressed) {
      m_uv_start_thumb_pressed = (*t_).get().start;
      m_uv_end_thumb_pressed = (*t_).get().end;
      mark_dirty();
    }
  }
  return *this;
}

zincgui::Slider& zincgui::Slider::texture_thumb_hovered(const std::string& id) {
  auto t_ = ui().get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != m_uv_start_thumb_hovered) {
      m_uv_start_thumb_hovered = (*t_).get().start;
      m_uv_end_thumb_hovered = (*t_).get().end;
      mark_dirty();
    }
  }
  return *this;
}

zincgui::Slider& zincgui::Slider::texture_thumb_idle(const std::string& id) {
  auto t_ = ui().get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != m_uv_start_thumb_idle) {
      m_uv_start_thumb_idle = (*t_).get().start;
      m_uv_end_thumb_idle = (*t_).get().end;
      mark_dirty();
    }
  }
  return *this;
}

zincgui::Slider& zincgui::Slider::texture_track_inactive(const std::string& id) {
  auto t_ = ui().get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != m_uv_start_track_inactive) {
      m_uv_start_track_inactive = (*t_).get().start;
      m_uv_end_track_inactive = (*t_).get().end;
      mark_dirty();
    }
  }
  return *this;
}

zincgui::Slider& zincgui::Slider::texture_track_active(const std::string& id) {
  auto t_ = ui().get_texture_atlas().get(id);
  if (t_.has_value()) {
    if ((*t_).get().start != m_uv_start_track_active) {
      m_uv_start_track_active = (*t_).get().start;
      m_uv_end_track_active = (*t_).get().end;
      mark_dirty();
    }
  }
  return *this;
}
