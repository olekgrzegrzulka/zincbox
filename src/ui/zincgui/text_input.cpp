#include "text_input.hpp"
#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "label.hpp"
#include "sprite.hpp"
#include "theme_config.hpp"
#include "ui/theme.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

zincgui::TextInput::TextInput(Root& ui_) : Sprite(ui_), label(add_child<Label>()), caret(add_child<ColorRect>()) {
  set_size(64, 24);
  set_nine_slice_margin(4);
  set_clip_children(true);
  label.set_anchor(Anchor::CENTER_LEFT);
  label.set_parent_anchor(Anchor::CENTER_LEFT);
  label.set_label_anchor(Anchor::CENTER_LEFT);
  label.set_pos({2, 0});
  caret.set_parent_anchor(Anchor::CENTER_LEFT);
  caret.set_anchor(Anchor::CENTER_CENTER);
  caret.set_color(theme::config().text_caret_color);
  caret.set_size({1, 14});
}

void zincgui::TextInput::input() {
  Sprite::input();

  bool backspace = Input::key_pressed(Input::Key::KEY_BACKSPACE);

  if (focused) {
    if (backspace) {
      if (backspace_clock <= 0.0f) {
        if (label.erase_last_character() && lambda_on_text_changed) { lambda_on_text_changed(); }
        backspace_clock += backspace_first_echo ? backspace_echo_length_initial : backspace_echo_length;
        backspace_first_echo = false;
        caret_blink_clock = std::min(caret_blink_clock + caret_blink_time, caret_blink_time);
        caret.set_is_updated(true);
      } else {
        backspace_clock -= 10.0f;
      }
    } else {
      backspace_clock = 0.0f;
      backspace_first_echo = true;
    }
  }

  if (focused) {
    auto typed_characters = Input::get_typed_characters();
    if (!typed_characters.empty()) {
      label.append_text(typed_characters);
      if (lambda_on_text_changed) { lambda_on_text_changed(); }
      caret_blink_clock = caret_blink_time;
      caret.set_is_drawn(true);
    }
    caret.set_pos(vec2i(label.get_x() + label.get_text_extents().x + 1, 0));
    caret_blink_clock -= 10.0f;

    if (caret_blink_clock <= 0) {
      caret.set_is_drawn(!caret.get_is_drawn());
      caret_blink_clock = std::min(caret_blink_clock + caret_blink_time, caret_blink_time);
    }

  } else {
    caret_blink_clock = 0.0f;
    caret.set_is_drawn(false);
  }
}

void zincgui::TextInput::update() {
  if (!focused) {
    set_texture("text_input_idle", false);
  } else {
    set_texture("text_input_focused", false);
  }
  Sprite::update();
}

void zincgui::TextInput::clear() {
  if (label.get_text().empty()) { return; }
  label.set_text("");
  if (lambda_on_text_changed) { lambda_on_text_changed(); }
}

void zincgui::TextInput::event(Input::InputEventMouseButton& ev) {
  bool mouse_on_widget_x = Input::get_mouse_x() >= get_position(Anchor::TOP_LEFT).x &&
                           Input::get_mouse_x() < get_position(Anchor::BOTTOM_RIGHT).x;
  bool mouse_on_widget_y = Input::get_mouse_y() >= get_position(Anchor::TOP_LEFT).y &&
                           Input::get_mouse_y() < get_position(Anchor::BOTTOM_RIGHT).y;

  bool mouse_hovering = mouse_on_widget_x && mouse_on_widget_y;
  // bool lmb_pressed = Input::mouse_pressed(Input::Mouse::MOUSE_BUTTON_LEFT);
  bool lmb_just_pressed = ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT && ev.action == Input::MouseAction::PRESS;
  bool rmb_just_pressed = ev.button == Input::MouseButton::MOUSE_BUTTON_RIGHT && ev.action == Input::MouseAction::PRESS;
  bool mmb_just_pressed =
    ev.button == Input::MouseButton::MOUSE_BUTTON_MIDDLE && ev.action == Input::MouseAction::PRESS;
  // bool lmb_just_released = Input::mouse_just_released(Input::Mouse::MOUSE_BUTTON_LEFT);

  if (mouse_hovering && lmb_just_pressed && !focused) {
    focused = true;
  } else if (!mouse_hovering && (lmb_just_pressed || rmb_just_pressed || mmb_just_pressed) && focused) {
    focused = false;
  }

  if (focused) { ev.handled = true; }
}

void zincgui::TextInput::event(Input::InputEventKey& ev) {
  if (!focused) { return; }
  if (ev.key == Input::Key::KEY_ESCAPE) {
    focused = false;
  } else {
    ev.handled = true;
  }
}
