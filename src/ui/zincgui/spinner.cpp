#include "spinner.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Spinner::Spinner(Root& ui_)
  : TextInput(ui_), m_button_increase(add_child<Button>("")), m_button_decrease(add_child<Button>("")),
    m_label_postfix(add_child<Label>()) {

  size(default_settings::spinner_size);
  if (default_settings::spinner_fixed_height) { fixed_height(height()); }

  is_focusable(true);

  m_button_decrease.parent_anchor(Anchor::LEFT)
    .nine_slice_margin(default_settings::spinner_button_nine_slice_margin)
    .anchor(Anchor::LEFT)
    .pos(default_settings::spinner_button_offset_x, 0)
    .add_image(default_settings::spinner_texture_decrease.icon)
    .texture_idle(default_settings::spinner_texture_decrease.button_idle)
    .texture_hovered(default_settings::spinner_texture_decrease.button_hovered)
    .texture_pressed(default_settings::spinner_texture_decrease.button_pressed)
    .texture_disabled(default_settings::spinner_texture_decrease.button_disabled);

  m_button_increase.parent_anchor(Anchor::RIGHT)
    .nine_slice_margin(default_settings::spinner_button_nine_slice_margin)
    .anchor(Anchor::RIGHT)
    .pos(-default_settings::spinner_button_offset_x, 0)
    .add_image(default_settings::spinner_texture_increase.icon)
    .texture_idle(default_settings::spinner_texture_increase.button_idle)
    .texture_hovered(default_settings::spinner_texture_increase.button_hovered)
    .texture_pressed(default_settings::spinner_texture_increase.button_pressed)
    .texture_disabled(default_settings::spinner_texture_increase.button_disabled);

  label().resize_to_text_extents(false).text(std::to_string(m_value));
  TextInput::on_text_changed([this]() { on_text_changed(); });

  m_label_postfix.anchor(Anchor::LEFT).parent_anchor(Anchor::LEFT).label_anchor(Anchor::LEFT);
}

void Spinner::update() {
  TextInput::update();

  if (m_buffer_changed && !is_focused()) {
    m_buffer_changed = false;

    i32 new_value = 0;
    auto str = label().text();

    if (str.empty()) {
      new_value = m_min_value;
    } else {
      try {
        new_value = std::stoi(str);
      } catch (const std::invalid_argument&) { new_value = m_value; } catch (const std::out_of_range&) {
        if (str.starts_with('-')) {
          new_value = std::numeric_limits<i32>::min();
        } else {
          new_value = std::numeric_limits<i32>::max();
        }
      }
    }

    if (!set_value_impl(new_value)) { label().text(std::to_string(new_value)); }
  }

  m_button_increase.fixed_width(m_button_increase.height())
    .fixed_height(height() - default_settings::spinner_button_height_padding)
    .size(m_button_increase.min_size());
  m_button_decrease.fixed_width(m_button_decrease.height())
    .fixed_height(height() - default_settings::spinner_button_height_padding)
    .size(m_button_increase.min_size());

  label()
    .height(height())
    .width(width() -
           (m_button_increase.width() + m_button_decrease.width() + (2 * default_settings::spinner_label_padding_x)))
    .pos({m_button_increase.width() + default_settings::spinner_label_padding_x, 0});

  auto scroll = Input::get_mouse_scroll();

  if (is_mouse_hovering() && is_focused()) {
    if (scroll.y > 0) {
      // focused = true;
      i32 lo = std::min(m_min_value, m_max_value);
      i32 hi = std::max(m_min_value, m_max_value);
      i32 new_value = std::clamp(m_value + m_button_step, lo, hi);
      set_value_impl(new_value);
    } else if (scroll.y < 0) {
      // focused = true;
      i32 lo = std::min(m_min_value, m_max_value);
      i32 hi = std::max(m_min_value, m_max_value);
      i32 new_value = std::clamp(m_value - m_button_step, lo, hi);
      set_value_impl(new_value);
    }
  }
  m_button_increase.disabled(m_value >= m_max_value);
  m_button_decrease.disabled(m_value <= m_min_value);

  bool increase_pressed = m_button_increase.state() == ButtonState::PRESSED;
  bool decrease_pressed = m_button_decrease.state() == ButtonState::PRESSED;
  if (increase_pressed || decrease_pressed) {
    // focused = true;
    if (m_buttons_clock <= 0) {
      if ((increase_pressed && set_value_impl(m_value + m_button_step)) ||
          (decrease_pressed && set_value_impl(m_value - m_button_step))) {
        m_buttons_clock =
          m_buttons_first_echo ? default_settings::spinner_echo_length_initial : default_settings::spinner_echo_length;
        m_buttons_first_echo = false;
        caret_blink_clock(caret_blink_time());
        caret().is_updated(true);
      }

    } else {
      m_buttons_clock -= 1;
    }
  } else {
    m_buttons_clock = 0;
    m_buttons_first_echo = true;
  }

  m_label_postfix.x(label().x() + label().text_extents().x + default_settings::spinner_postfix_offset_x);
}

void zincgui::Spinner::event(Input::InputEventMouseButton&) {}

void zincgui::Spinner::event(Input::InputEventMouseScroll& ev) {
  if (is_focused() && is_mouse_hovering()) { ev.handled = true; }
}

void Spinner::on_text_changed() {
  auto str = label().text();
  bool typed_minus = str.ends_with('-');
  if (typed_minus) {
    str.erase(str.length() - 1, 1);
    if (str.starts_with('-')) {
      str.erase(0, 1);
    } else {
      str = '-' + str;
    }
  }
  label().text(str);
  m_buffer_changed = true;
}

bool Spinner::set_value_impl(i32 value_) {
  i32 lo = std::min(m_min_value, m_max_value);
  i32 hi = std::max(m_min_value, m_max_value);
  i32 new_value = std::clamp(value_, lo, hi);
  if (m_value != new_value) {
    m_value = new_value;
    label().text(std::to_string(m_value));
    on_text_changed();
    if (m_on_value_changed) { m_on_value_changed(); }
    return true;
  }
  return false;
}

void Spinner::set_postfix_impl(const std::string& postfix_) {
  if (m_postfix != postfix_) { m_postfix = postfix_; }
  m_label_postfix.text(m_postfix);
}

void Spinner::set_min_value_impl(i32 min_value_) {
  if (min_value_ == m_max_value) { return; }
  m_min_value = min_value_;
  set_value_impl(m_value);
}

void Spinner::set_max_value_impl(i32 max_value_) {
  if (max_value_ == m_max_value) { return; }
  m_max_value = max_value_;
  set_value_impl(m_value);
}
