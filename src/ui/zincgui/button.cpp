#include "button.hpp"
#include <optional>
#include <utility>
#include "common/color.hpp"
#include "common/logger.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Button::Button(Root& ui_) : Sprite::Sprite(ui_), m_label(add_child<Label>()) { init(); }

Button::Button(Root& ui_, std::string_view label_) : Sprite::Sprite(ui_), m_label(add_child<Label>(label_)) { init(); }

void zincgui::Button::update() { Sprite::update(); }

void zincgui::Button::event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (m_state == DISABLED) { return; }

  bool lmb_pressed = ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS;
  bool lmb_released = ev.button == MOUSE_BUTTON_LEFT && ev.action == RELEASE;

  bool rmb_pressed = ev.button == MOUSE_BUTTON_RIGHT && ev.action == PRESS;
  bool rmb_released = ev.button == MOUSE_BUTTON_RIGHT && ev.action == RELEASE;

  if (is_mouse_hovering() && rmb_pressed) {
    m_rmb_held = true;
    state(PRESSED);
    ev.handled = true;
  }

  if (is_mouse_hovering() && m_rmb_held && rmb_released) {
    if (m_state != ButtonState::DISABLED && m_on_press_rmb) { m_on_press_rmb(); }
    m_rmb_held = false;
  }

  if (is_mouse_hovering() && lmb_pressed) {
    m_mouse_pressed = true;
    state(PRESSED);
    ev.handled = true;
  }

  if (is_mouse_hovering() && m_mouse_pressed && lmb_released) {
    m_mouse_pressed = false;
    if (m_switch_mode) {
      is_switched(!m_is_switched);

    } else {
      state(HOVERED);
      pressed();
      ev.handled = true;
    }
  }

  if (!is_mouse_hovering() && lmb_released && m_state == PRESSED) {
    m_mouse_pressed = false;
    if (m_switch_mode) {
      if (m_is_switched) {
        state(PRESSED);
      } else {
        state(IDLE);
      }

    } else {
      state(IDLE);
    }
  }
}

void zincgui::Button::event(Input::InputEventMouseMove& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (m_state == DISABLED) { return; }

  if (is_mouse_hovering()) {
    if (!m_switch_mode && m_state == IDLE) { state(HOVERED); }
    if (m_switch_mode && (m_state == IDLE || m_state == PRESSED)) { state(HOVERED); }
  }

  if (is_mouse_hovering() && m_mouse_pressed) {
    if (m_switch_mode) {
      state(m_is_switched ? IDLE : PRESSED);
    } else {
      state(PRESSED);
    }
  }

  if (!is_mouse_hovering()) {
    m_mouse_pressed = false;
    if (m_switch_mode) {
      state(m_is_switched ? PRESSED : IDLE);
    } else {
      state(IDLE);
    }
  }
}

void zincgui::Button::event(Input::InputEventMouseScroll&) {}

void Button::pressed() {
  if (m_on_press) { m_on_press(); }
}

void Button::depressed() {
  if (m_on_depress) { m_on_depress(); }
}

void Button::on_state_changed(ButtonState /* prev_state */) {
  if (m_state == ButtonState::HOVERED) {
    sprite_hovered();
    label().text_color(default_settings::text_color);
  } else if (m_state == ButtonState::IDLE) {
    sprite_idle();
    label().text_color(default_settings::text_color);
  } else if (m_state == ButtonState::PRESSED) {
    sprite_pressed();
    label().text_color(default_settings::text_color);
  } else if (m_state == ButtonState::DISABLED) {
    sprite_disabled();
    label().text_color(default_settings::text_color_disabled);
  }
}

void Button::init() {
  hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH)
    .size(default_settings::button_size);

  if (default_settings::button_fixed_height) { fixed_height(height()); }

  texture_idle(default_settings::button_texture_idle)
    .texture_hovered(default_settings::button_texture_hovered)
    .texture_pressed(default_settings::button_texture_pressed)
    .texture_disabled(default_settings::button_texture_disabled)
    .nine_slice_margin(default_settings::button_nine_slice_margin)
    .nine_slice_scale(default_settings::button_nine_slice_scale);

  m_label.text_color(default_settings::text_color)
         .parent_anchor(Anchor::CENTER_CENTER)
         .anchor(Anchor::CENTER_CENTER);

  sprite_idle();
}

void zincgui::Button::add_image_impl(std::string_view id, bool resize_to_texture_size) {
  if (!m_image) {
    m_image = &add_child<Sprite>();
    m_image->anchor(Anchor::CENTER)
            .parent_anchor(Anchor::CENTER)
            .nine_slice_margin(0);
    m_label.is_drawn(false);
  }

  m_image->texture(std::string(id), resize_to_texture_size);
}

void zincgui::Button::set_texture_idle_impl(std::string id) {
  auto val = ui().get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  m_uv_start_idle = val->get().start;
  m_uv_end_idle = val->get().end;
  texture_width(val->get().width).texture_height(val->get().height);
  if (m_state == ButtonState::IDLE) { sprite_idle(); }
}

void zincgui::Button::set_texture_hovered_impl(std::string id) {
  auto val = ui().get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  m_uv_start_hovered = val->get().start;
  m_uv_end_hovered = val->get().end;
  texture_width(val->get().width).texture_height(val->get().height);
  if (m_state == ButtonState::HOVERED) { sprite_hovered(); }
}

void zincgui::Button::set_texture_pressed_impl(std::string id) {
  auto val = ui().get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  m_uv_start_pressed = val->get().start;
  m_uv_end_pressed = val->get().end;
  texture_width(val->get().width).texture_height(val->get().height);
  if (m_state == ButtonState::PRESSED) { sprite_pressed(); }
}

void zincgui::Button::set_texture_disabled_impl(std::string id) {
  auto val = ui().get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  m_uv_start_disabled = val->get().start;
  m_uv_end_disabled = val->get().end;
  texture_width(val->get().width).texture_height(val->get().height);
  if (m_state == ButtonState::DISABLED) { sprite_disabled(); }
}
