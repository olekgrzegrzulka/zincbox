#include "button.hpp"
#include <optional>
#include <utility>
#include "common/color.hpp"
#include "common/logger.hpp"
#include "ui/theme.hpp" // FIXME zincgui should not depend on ui/theme.hpp
#include "ui/theme_config.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Button::Button(Root& ui_) : Sprite::Sprite(ui_), label(add_child<Label>()) { init(); }

Button::Button(Root& ui_, std::string_view label_) : Sprite::Sprite(ui_), label(add_child<Label>(label_)) { init(); }

void Button::init() {
  set_size(64, 24);
  set_texture_idle("button_idle");
  set_texture_hovered("button_hovered");
  set_texture_pressed("button_pressed");
  set_texture_disabled("button_disabled");
  set_nine_slice_margin(10.0f);
  set_nine_slice_scale(1.0f);

  label.set_text_color(theme::config().text_color);
  label.set_parent_anchor(Anchor::CENTER_CENTER);
  label.set_anchor(Anchor::CENTER_CENTER);

  set_sprite_idle();
}

Label& Button::get_label() { return label; }
const Label& Button::get_label() const { return label; }

void Button::draw() { Sprite::draw(); }

void Button::on_press(std::function<void()> lambda_press_) { lambda_press = std::move(lambda_press_); }
void Button::on_depress(std::function<void()> lambda_depress_) { lambda_depress = std::move(lambda_depress_); }
void Button::on_press_rmb(std::function<void()> lambda_press_rmb_) { lambda_press_rmb = std::move(lambda_press_rmb_); }

// Returns true if button's state was changed
bool Button::set_state(ButtonState state_) {
  if (state_ == state) { return false; }
  auto prev_state = state;
  state = state_;
  on_state_changed(prev_state);
  return true;
}

void zincgui::Button::update() { Sprite::update(); }

void zincgui::Button::add_image(std::string_view id, bool resize_to_texture_size) {
  if (!image) {
    image = &add_child<Sprite>();
    image->set_anchor(Anchor::CENTER);
    image->set_parent_anchor(Anchor::CENTER);
    image->set_nine_slice_margin(0);
    label.set_is_drawn(false);
  }

  image->set_texture(std::string(id), resize_to_texture_size);
}

void zincgui::Button::set_texture_idle(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  uv_start_idle = val->get().start;
  uv_end_idle = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::IDLE) { set_sprite_idle(); }
}
void zincgui::Button::set_texture_hovered(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  uv_start_hovered = val->get().start;
  uv_end_hovered = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::HOVERED) { set_sprite_hovered(); }
}
void zincgui::Button::set_texture_pressed(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  uv_start_pressed = val->get().start;
  uv_end_pressed = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::PRESSED) { set_sprite_pressed(); }
}
void zincgui::Button::set_texture_disabled(std::string id) {
  auto val = ui.get_texture_atlas().get(id);
  if (!val.has_value()) {
    out::debug_warn("atlas texture not found: {}", id);
    return;
  }
  uv_start_disabled = val->get().start;
  uv_end_disabled = val->get().end;
  texture_width = val->get().width;
  texture_height = val->get().height;
  if (state == ButtonState::DISABLED) { set_sprite_disabled(); }
}

void zincgui::Button::event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (state == DISABLED) { return; }

  bool lmb_pressed = ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS;
  bool lmb_released = ev.button == MOUSE_BUTTON_LEFT && ev.action == RELEASE;

  bool rmb_pressed = ev.button == MOUSE_BUTTON_RIGHT && ev.action == PRESS;
  bool rmb_released = ev.button == MOUSE_BUTTON_RIGHT && ev.action == RELEASE;

  if (mouse_hovering && rmb_pressed) {
    rmb_held = true;
    set_state(PRESSED);
    ev.handled = true;
  }

  if (mouse_hovering && rmb_held && rmb_released) {
    if (state != ButtonState::DISABLED && lambda_press_rmb) { lambda_press_rmb(); }
    rmb_held = false;
  }

  if (mouse_hovering && lmb_pressed) {
    mouse_pressed = true;
    set_state(PRESSED);
    ev.handled = true;
  }

  if (mouse_hovering && mouse_pressed && lmb_released) {
    mouse_pressed = false;
    if (switch_mode) {
      set_is_switched(!is_switched);

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

void zincgui::Button::event(Input::InputEventMouseMove& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  using enum ButtonState;

  if (state == DISABLED) { return; }

  mouse_hovering = is_mouse_hovering(ev.to);
  if (mouse_hovering) {
    if (!switch_mode && state == IDLE) { set_state(HOVERED); }
    if (switch_mode && (state == IDLE || state == PRESSED)) { set_state(HOVERED); }
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

void zincgui::Button::event(Input::InputEventMouseScroll&) {}

void Button::on_state_changed(ButtonState /* prev_state */) {
  if (state == ButtonState::HOVERED) {
    set_sprite_hovered();
    get_label().set_text_color(theme::config().text_color);
  } else if (state == ButtonState::IDLE) {
    set_sprite_idle();
    get_label().set_text_color(theme::config().text_color);
  } else if (state == ButtonState::PRESSED) {
    set_sprite_pressed();
    get_label().set_text_color(theme::config().text_color);
  } else if (state == ButtonState::DISABLED) {
    set_sprite_disabled();
    get_label().set_text_color(theme::config().text_color_disabled);
  }
}
