#pragma once
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include "button.hpp"
#include "common/input.hpp"
#include "common/utf.hpp"
#include "label.hpp"
#include "sprite.hpp"
#include "text_input.hpp"
#include "ui/zb_widgets.hpp"
#include "widget.hpp"

class Spinner : public TextInput {
  public:
    Spinner(UI& ui_)
      : TextInput(ui_), button_increase(add_child<ZincboxButton>("spinner_increase")),
        button_decrease(add_child<ZincboxButton>("spinner_decrease")), label_postfix(add_child<Label>()) {

      button_decrease.set_parent_anchor(Anchor::LEFT);
      button_decrease.set_nine_slice_margin(2.0f);
      button_decrease.set_anchor(Anchor::LEFT);
      button_decrease.set_pos(1, 0);

      button_increase.set_parent_anchor(Anchor::RIGHT);
      button_increase.set_nine_slice_margin(2.0f);
      button_increase.set_anchor(Anchor::RIGHT);
      button_increase.set_pos(-1, 0);

      label.set_pos({button_increase.get_width() + 2, 0});
      label.set_resize_to_text_extents(false);

      label.set_text(std::to_string(value));
      set_on_text_changed([this]() { on_text_changed(); });

      label_postfix.set_anchor(Anchor::LEFT);
      label_postfix.set_parent_anchor(Anchor::LEFT);
      label_postfix.set_label_anchor(Anchor::LEFT);
    }

    void on_text_changed() {
      auto str = label.get_text();
      bool typed_minus = str.ends_with(U'-');
      if (typed_minus) {
        str.erase(str.length() - 1, 1);
        if (str.starts_with(U'-')) {
          str.erase(0, 1);
        } else {
          str = U'-' + str;
        }
      }
      label.set_text(str);
      buffer_changed = true;
    }

    void update() override {
      TextInput::update();

      if (buffer_changed && !focused) {
        buffer_changed = false;

        i32 new_value = 0;
        auto str = label.get_text();

        if (str.empty()) {
          new_value = min_value;
        } else {
          try {
            new_value = std::stoi(utf32_to_utf8(str));
          } catch (std::invalid_argument) { new_value = value; } catch (std::out_of_range) {
            if (str.starts_with('-')) {
              new_value = std::numeric_limits<i32>::min();
            } else {
              new_value = std::numeric_limits<i32>::max();
            }
          }
        }

        if (!set_value(new_value)) { label.set_text(std::to_string(new_value)); }
      }

      label.set_height(height);
      label.set_width(width - (button_increase.get_width() + button_decrease.get_width() + 4));

      button_increase.set_height(height - 2);
      button_decrease.set_height(height - 2);

      auto scroll = Input::get_mouse_scroll();

      if (is_mouse_hovering()) {
        if (scroll.y > 0) {
          focused = true;
          i32 new_value = std::clamp(value + button_step, min_value, max_value);
          set_value(new_value);
        } else if (scroll.y < 0) {
          focused = true;
          i32 new_value = std::clamp(value - button_step, min_value, max_value);
          set_value(new_value);
        }
      }
      button_increase.set_disabled(value >= max_value);
      button_decrease.set_disabled(value <= min_value);

      bool increase_pressed = button_increase.get_state() == ButtonState::PRESSED;
      bool decrease_pressed = button_decrease.get_state() == ButtonState::PRESSED;
      if (increase_pressed || decrease_pressed) {
        focused = true;
        if (buttons_clock <= 0) {
          if ((increase_pressed && set_value(value + button_step)) ||
              (decrease_pressed && set_value(value - button_step))) {
            buttons_clock = buttons_first_echo ? buttons_echo_length_initial : buttons_echo_length;
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

      label_postfix.set_x(label.get_x() + label.get_text_extents().x + 1);
    }

  protected:
    Button& button_increase;
    Button& button_decrease;
    Label& label_postfix;

    std::function<void()> lambda_value_changed = nullptr;

    i32 value = 0;
    i32 min_value = 0;
    i32 max_value = 100;
    bool buffer_changed = false;
    std::u32string postfix = U"";
    i32 button_step = 1;

    static constexpr i32 buttons_echo_length_initial = 40;
    static constexpr i32 buttons_echo_length = 2;
    i32 buttons_clock = 0;
    bool buttons_first_echo = true;

  public:
    WIDGET_DEF_GETTER(value);
    WIDGET_DEF_GETTER(min_value);
    WIDGET_DEF_GETTER(max_value);
    WIDGET_DEF_GETTER(button_step);
    void set_postfix(const std::u32string& postfix_) {
      if (postfix != postfix_) { postfix = postfix_; }
      label_postfix.set_text(postfix);
    }

    bool set_value(i32 value_) {
      i32 new_value = std::clamp(value_, min_value, max_value);
      if (value != new_value) {
        value = new_value;
        label.set_text(std::to_string(value));
        on_text_changed();
        if (lambda_value_changed) { lambda_value_changed(); }
        return true;
      }
      return false;
    }

    void on_value_changed(std::function<void()> lambda_value_changed_) {
      lambda_value_changed = std::move(lambda_value_changed_);
    }

    void set_min_value(i32 min_value_) {
      if (min_value_ == max_value) { return; }
      min_value = min_value_;
      set_value(value);
    }

    void set_max_value(i32 max_value_) {
      if (max_value_ == max_value) { return; }
      max_value = max_value_;
      set_value(value);
    }

    WIDGET_DEF_SETTER(button_step);
};
