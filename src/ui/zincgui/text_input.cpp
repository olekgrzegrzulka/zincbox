#include "text_input.hpp"
#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "ui/theme.hpp" // FIXME zincgui should not depend on ui/theme.hpp
#include "ui/theme_config.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

zincgui::TextInput::TextInput(Root& ui_) : Sprite(ui_), label(add_child<Label>()), caret(add_child<ColorRect>()) {
  set_size(64, 24);
  set_nine_slice_margin(10.0f);
  set_clip_children(true);
  label.set_anchor(Anchor::CENTER_LEFT);
  label.set_parent_anchor(Anchor::CENTER_LEFT);
  label.set_label_anchor(Anchor::CENTER_LEFT);
  label.set_pos({2, 0});
  caret.set_parent_anchor(Anchor::CENTER_LEFT);
  caret.set_anchor(Anchor::CENTER_CENTER);
  caret.set_color(theme::config().text_caret_color);
  caret.set_size({1, 14});
  set_is_focusable(true);
}

void zincgui::TextInput::input() {
  Sprite::input();

  bool backspace = Input::key_pressed(Input::Key::KEY_BACKSPACE);

  if (is_focused()) {
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

  if (is_focused()) {
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
  if (is_focused()) {
    set_texture("text_input_focused", false);
  } else {
    set_texture("text_input_idle", false);
  }
  Sprite::update();
}

void zincgui::TextInput::clear() {
  if (label.get_text().empty()) { return; }
  label.set_text("");
  if (lambda_on_text_changed) { lambda_on_text_changed(); }
}

void zincgui::TextInput::event(Input::InputEventMouseButton& ev) {}

void zincgui::TextInput::event(Input::InputEventKey& ev) {
  if (!is_focused()) { return; }
  if (ev.key == Input::Key::KEY_ESCAPE || ev.key == Input::Key::KEY_ENTER || ev.key == Input::Key::KEY_KP_ENTER ||
      ev.key == Input::Key::KEY_ENTER2) {
    ui.set_focused_widget(nullptr);
  } else {
    ev.handled = true;
  }
}
