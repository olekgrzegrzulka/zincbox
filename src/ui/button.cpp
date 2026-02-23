#include "button.hpp"
#include "../input.hpp"
#include "ui.hpp"

void Button::update() {
  Sprite::update();

  if (state == ButtonState::PRESSED && offset_label_on_press) {
    label.set_x(1);
    label.set_y(1);
  } else {
    label.set_x(0);
    label.set_y(0);
  }
  label.set_parent_anchor(Anchor::CENTER_CENTER);
  label.set_anchor(Anchor::CENTER_CENTER);
}

void Button::press() {
  if (state == ButtonState::DISABLED) { return; }

  if (!switch_mode) {
    set_state(ButtonState::PRESSED);
    pressed();
  } else {
    set_is_switched(!is_switched);
    if (is_switched) {
      set_state(ButtonState::PRESSED);
      pressed();
    } else {
      set_state(ButtonState::IDLE);
      depressed();
    }
  }
}

void Button::set_texture_idle(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    debug_warn("atlas texture not found: " + id);
    return;
  }
  uv_start_idle = val->get().start;
  uv_end_idle = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::IDLE) { dirty = true; } // FIXME check if texture actually changed
}
void Button::set_texture_hovered(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    debug_warn("atlas texture not found: " + id);
    return;
  }
  uv_start_hovered = val->get().start;
  uv_end_hovered = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::HOVERED) { dirty = true; } // FIXME check if texture actually changed
}
void Button::set_texture_pressed(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    debug_warn("atlas texture not found: " + id);
    return;
  }
  uv_start_pressed = val->get().start;
  uv_end_pressed = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::PRESSED) { dirty = true; } // FIXME check if texture actually changed
}
void Button::set_texture_disabled(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    debug_warn("atlas texture not found: " + id);
    return;
  }
  uv_start_disabled = val->get().start;
  uv_end_disabled = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::DISABLED) { dirty = true; } // FIXME check if texture actually changed
}

void Button::handle_event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (state == DISABLED) { return; }

  bool lmb_pressed = ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS;
  bool lmb_released = ev.button == MOUSE_BUTTON_LEFT && ev.action == RELEASE;

  if (mouse_hovering && lmb_pressed) {
    mouse_pressed = true;
    set_state(PRESSED);
    ev.handled = true;
  }

  if (mouse_hovering && mouse_pressed && lmb_released) {
    mouse_pressed = false;
    if (switch_mode) {
      set_is_switched(!is_switched);
      if (is_switched) {
        set_state(PRESSED);
        pressed();
        ev.handled = true;
      } else {
        set_state(HOVERED);
        depressed();
        ev.handled = true;
      }

    } else {
      set_state(HOVERED);
      pressed();
      ev.handled = true;
    }
  }

  if (!mouse_hovering && lmb_released && state == PRESSED) {
    mouse_pressed = false;
    if (switch_mode) {
      if (is_switched) {
        set_state(PRESSED);
      } else {
        set_state(IDLE);
      }

    } else {
      set_state(IDLE);
    }
  }
}

void Button::handle_event(Input::InputEventMouseMove& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (state == DISABLED) { return; }

  mouse_hovering = is_mouse_hovering(ev.to);
  if (mouse_hovering) {
    if (!switch_mode && state == IDLE) {
      set_state(HOVERED);
    }
    if (switch_mode && (state == IDLE || state == PRESSED)) {
      set_state(HOVERED);
    }
  }

  if (mouse_hovering && mouse_pressed) {
    if (switch_mode) {
      set_state(is_switched ? IDLE : PRESSED);
    } else {
      set_state(PRESSED);
    }
  }

  if (!mouse_hovering) {
    mouse_pressed = false;
    if (switch_mode) {
      set_state(is_switched ? PRESSED : IDLE);
    } else {
      set_state(IDLE);
    }
  }
}

void Button::handle_event(Input::InputEventMouseScroll&) {
}