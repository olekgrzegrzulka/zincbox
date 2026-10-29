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

zincgui::TextInput::TextInput(Root& ui_) :
    Sprite(ui_), m_label(add_child<Label>()), m_caret(add_child<ColorRect>()) {
  hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH)
    .size(default_settings::text_input_size);
  if (default_settings::text_input_fixed_height) { fixed_height(height()); }
  nine_slice_margin(default_settings::text_input_nine_slice_margin)
    .clip_children(true);

  m_label.anchor(Anchor::CENTER_LEFT)
    .parent_anchor(Anchor::CENTER_LEFT)
    .label_anchor(Anchor::CENTER_LEFT)
    .pos(default_settings::text_input_label_offset);

  m_caret.parent_anchor(Anchor::CENTER_LEFT)
    .anchor(Anchor::CENTER_CENTER)
    .color(default_settings::text_caret_color)
    .size(default_settings::text_input_caret_size);

  is_focusable(true)
    .texture(default_settings::text_input_texture_idle, false);
}

void zincgui::TextInput::input() {
  Sprite::input();

  bool backspace = Input::key_pressed(Input::Key::KEY_BACKSPACE);

  if (is_focused()) {
    if (backspace) {
      if (m_backspace_clock <= 0.0f) {
        if (m_label.erase_last_character() && m_on_text_changed) { m_on_text_changed(); }
        m_backspace_clock += m_backspace_first_echo ? m_backspace_echo_length_initial : m_backspace_echo_length;
        m_backspace_first_echo = false;
        m_caret_blink_clock = std::min(m_caret_blink_clock + m_caret_blink_time, m_caret_blink_time);
        m_caret.is_updated(true);
      } else {
        m_backspace_clock -= default_settings::text_input_clock_step;
      }
    } else {
      m_backspace_clock = 0.0f;
      m_backspace_first_echo = true;
    }
  }

  if (is_focused()) {
    auto typed_characters = Input::get_typed_characters();
    if (!typed_characters.empty()) {
      m_label.append_text(typed_characters);
      if (m_on_text_changed) { m_on_text_changed(); }
      m_caret_blink_clock = m_caret_blink_time;
      m_caret.is_drawn(true);
    }
    m_caret.pos(vec2i(m_label.x() + m_label.text_extents().x + default_settings::text_input_caret_offset_x, 0));
    m_caret_blink_clock -= default_settings::text_input_clock_step;

    if (m_caret_blink_clock <= 0) {
      m_caret.is_drawn(!m_caret.is_drawn());
      m_caret_blink_clock = std::min(m_caret_blink_clock + m_caret_blink_time, m_caret_blink_time);
    }

  } else {
    m_caret_blink_clock = 0.0f;
    m_caret.is_drawn(false);
  }
}

void zincgui::TextInput::update() { Sprite::update(); }

void zincgui::TextInput::on_focused() { texture(default_settings::text_input_texture_focused, false); }

void zincgui::TextInput::on_unfocused() { texture(default_settings::text_input_texture_idle, false); }

void zincgui::TextInput::event(Input::InputEventMouseButton& ev) {}

void zincgui::TextInput::event(Input::InputEventKey& ev) {
  if (!is_focused()) { return; }
  if (ev.key == Input::Key::KEY_ESCAPE || ev.key == Input::Key::KEY_ENTER || ev.key == Input::Key::KEY_KP_ENTER ||
      ev.key == Input::Key::KEY_ENTER2) {
    ui().set_focused_widget(nullptr);
  } else {
    ev.handled = true;
  }
}

void zincgui::TextInput::clear() {
  if (m_label.text().empty()) { return; }
  m_label.text("");
  if (m_on_text_changed) { m_on_text_changed(); }
}
