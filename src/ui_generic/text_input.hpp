#pragma once
#include "common/input.hpp"
#include "label.hpp"
#include "sprite.hpp"

class TextInput : public Sprite {
  public:
    TextInput(UI& ui_);
    void update();
    void clear();
    void handle_input(Input::InputEventKey&);

  protected:
    std::function<void()> lambda_on_text_changed;

  protected:
    bool focused = false;

    static constexpr float caret_blink_time = 250.0f;
    float caret_blink_clock = caret_blink_time;

    static constexpr float backspace_echo_length_initial = 250.0f;
    static constexpr float backspace_echo_length = 15.0f;
    float backspace_clock = 0.0f;
    bool backspace_first_echo = true;

  public:
    void set_on_text_changed(std::function<void()> l) {
      lambda_on_text_changed = l;
    }
    Label& label;
    Sprite& caret;
};
