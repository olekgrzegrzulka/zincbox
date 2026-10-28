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
  : TextInput(ui_), button_increase(add_child<Button>("")), button_decrease(add_child<Button>("")),
    label_postfix(add_child<Label>()) {

  set_size(default_settings::spinner_size);
  if (default_settings::spinner_fixed_height) { set_fixed_height(height); }

  set_is_focusable(true);

  button_decrease.set_parent_anchor(Anchor::LEFT);
  button_decrease.set_nine_slice_margin(default_settings::spinner_button_nine_slice_margin);
  button_decrease.set_anchor(Anchor::LEFT);
  button_decrease.set_pos(default_settings::spinner_button_offset_x, 0);
  button_decrease.add_image(default_settings::spinner_texture_decrease_icon);
  button_decrease.set_texture_idle(default_settings::spinner_texture_button_idle);
  button_decrease.set_texture_hovered(default_settings::spinner_texture_button_hovered);
  button_decrease.set_texture_pressed(default_settings::spinner_texture_button_pressed);
  button_decrease.set_texture_disabled(default_settings::spinner_texture_button_disabled);

  button_increase.set_parent_anchor(Anchor::RIGHT);
  button_increase.set_nine_slice_margin(default_settings::spinner_button_nine_slice_margin);
  button_increase.set_anchor(Anchor::RIGHT);
  button_increase.set_pos(-default_settings::spinner_button_offset_x, 0);
  button_increase.add_image(default_settings::spinner_texture_increase_icon);
  button_increase.set_texture_idle(default_settings::spinner_texture_button_idle);
  button_increase.set_texture_hovered(default_settings::spinner_texture_button_hovered);
  button_increase.set_texture_pressed(default_settings::spinner_texture_button_pressed);
  button_increase.set_texture_disabled(default_settings::spinner_texture_button_disabled);

  label.set_resize_to_text_extents(false);

  label.set_text(std::to_string(value));
  set_on_text_changed([this]() { on_text_changed(); });

  label_postfix.set_anchor(Anchor::LEFT);
  label_postfix.set_parent_anchor(Anchor::LEFT);
  label_postfix.set_label_anchor(Anchor::LEFT);
}

void Spinner::on_text_changed() {
  auto str = label.get_text();
  bool typed_minus = str.ends_with('-');
  if (typed_minus) {
    str.erase(str.length() - 1, 1);
    if (str.starts_with('-')) {
      str.erase(0, 1);
    } else {
      str = '-' + str;
    }
  }
  label.set_text(str);
  buffer_changed = true;
}

void Spinner::update() {
  TextInput::update();

  if (buffer_changed && !is_focused()) {
    buffer_changed = false;

    i32 new_value = 0;
    auto str = label.get_text();

    if (str.empty()) {
      new_value = min_value;
    } else {
      try {
        new_value = std::stoi(str);
      } catch (const std::invalid_argument&) { new_value = value; } catch (const std::out_of_range&) {
        if (str.starts_with('-')) {
          new_value = std::numeric_limits<i32>::min();
        } else {
          new_value = std::numeric_limits<i32>::max();
        }
      }
    }

    if (!set_value(new_value)) { label.set_text(std::to_string(new_value)); }
  }

  button_increase.set_fixed_width(button_increase.get_height());
  button_increase.set_fixed_height(height - default_settings::spinner_button_height_padding);
  button_increase.set_size(button_increase.get_min_size());
  button_decrease.set_fixed_width(button_decrease.get_height());
  button_decrease.set_fixed_height(height - default_settings::spinner_button_height_padding);
  button_decrease.set_size(button_increase.get_min_size());

  label.set_height(height);
  label.set_width(width - (button_increase.get_width() + button_decrease.get_width() +
                           (2 * default_settings::spinner_label_padding_x)));
  label.set_pos({button_increase.get_width() + default_settings::spinner_label_padding_x, 0});

  auto scroll = Input::get_mouse_scroll();

  if (is_mouse_hovering() && is_focused()) {
    if (scroll.y > 0) {
      // focused = true;
      i32 lo = std::min(min_value, max_value);
      i32 hi = std::max(min_value, max_value);
      i32 new_value = std::clamp(value + button_step, lo, hi);
      set_value(new_value);
    } else if (scroll.y < 0) {
      // focused = true;
      i32 lo = std::min(min_value, max_value);
      i32 hi = std::max(min_value, max_value);
      i32 new_value = std::clamp(value - button_step, lo, hi);
      set_value(new_value);
    }
  }
  button_increase.set_disabled(value >= max_value);
  button_decrease.set_disabled(value <= min_value);

  bool increase_pressed = button_increase.get_state() == ButtonState::PRESSED;
  bool decrease_pressed = button_decrease.get_state() == ButtonState::PRESSED;
  if (increase_pressed || decrease_pressed) {
    // focused = true;
    if (buttons_clock <= 0) {
      if ((increase_pressed && set_value(value + button_step)) ||
          (decrease_pressed && set_value(value - button_step))) {
        buttons_clock =
          buttons_first_echo ? default_settings::spinner_echo_length_initial : default_settings::spinner_echo_length;
        buttons_first_echo = false;
        caret_blink_clock = caret_blink_time;
        caret.set_is_updated(true);
      }

    } else {
      buttons_clock -= 1;
    }
  } else {
    buttons_clock = 0;
    buttons_first_echo = true;
  }

  label_postfix.set_x(label.get_x() + label.get_text_extents().x + default_settings::spinner_postfix_offset_x);
}

void zincgui::Spinner::event(Input::InputEventMouseButton&) {}

void zincgui::Spinner::event(Input::InputEventMouseScroll& ev) {
  if (is_focused() && is_mouse_hovering()) { ev.handled = true; }
}

void Spinner::set_postfix(const std::string& postfix_) {
  if (postfix != postfix_) { postfix = postfix_; }
  label_postfix.set_text(postfix);
}

bool Spinner::set_value(i32 value_) {
  i32 lo = std::min(min_value, max_value);
  i32 hi = std::max(min_value, max_value);
  i32 new_value = std::clamp(value_, lo, hi);
  if (value != new_value) {
    value = new_value;
    label.set_text(std::to_string(value));
    on_text_changed();
    if (lambda_value_changed) { lambda_value_changed(); }
    return true;
  }
  return false;
}

void Spinner::on_value_changed(std::function<void()> lambda_value_changed_) {
  lambda_value_changed = std::move(lambda_value_changed_);
}

void Spinner::set_min_value(i32 min_value_) {
  if (min_value_ == max_value) { return; }
  min_value = min_value_;
  set_value(value);
}

void Spinner::set_max_value(i32 max_value_) {
  if (max_value_ == max_value) { return; }
  max_value = max_value_;
  set_value(value);
}
