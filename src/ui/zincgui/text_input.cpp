#include "text_input.hpp"
#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

zincgui::TextInput::TextInput(Root& ui_) : Sprite(ui_), label(add_child<Label>()), caret(add_child<ColorRect>()) {
  set_hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH);
  set_size(default_settings::text_input_size);
  if (default_settings::text_input_fixed_height) { set_fixed_height(height); }
  set_nine_slice_margin(default_settings::text_input_nine_slice_margin);
  set_clip_children(true);
  label.set_anchor(Anchor::CENTER_LEFT);
  label.set_parent_anchor(Anchor::CENTER_LEFT);
  label.set_label_anchor(Anchor::CENTER_LEFT);
  label.set_pos(default_settings::text_input_label_offset);
  caret.set_parent_anchor(Anchor::CENTER_LEFT);
  caret.set_anchor(Anchor::CENTER_CENTER);
  caret.set_color(default_settings::text_caret_color);
  caret.set_size(default_settings::text_input_caret_size);
  set_is_focusable(true);
  set_texture(default_settings::text_input_texture_idle, false);
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
        backspace_clock -= default_settings::text_input_clock_step;
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
    caret.set_pos(vec2i(label.get_x() + label.get_text_extents().x + default_settings::text_input_caret_offset_x, 0));
    caret_blink_clock -= default_settings::text_input_clock_step;

    if (caret_blink_clock <= 0) {
      caret.set_is_drawn(!caret.get_is_drawn());
      caret_blink_clock = std::min(caret_blink_clock + caret_blink_time, caret_blink_time);
    }

  } else {
    caret_blink_clock = 0.0f;
    caret.set_is_drawn(false);
  }
}

void zincgui::TextInput::on_focused() { set_texture(default_settings::text_input_texture_focused, false); }

void zincgui::TextInput::on_unfocused() { set_texture(default_settings::text_input_texture_idle, false); }

void zincgui::TextInput::update() { Sprite::update(); }

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
