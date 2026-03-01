#pragma once
#include "label.hpp"
#include "sprite.hpp"

class TextInput : public Sprite {
  public:
    TextInput(UI& ui_);
    void update();

  protected:
    virtual void on_text_changed() {}

  protected:
    bool focused = false;

    static constexpr i32 caret_blink_time = 40;
    i32 caret_blink_clock = caret_blink_time;

    static constexpr i32 backspace_echo_length_initial = 40;
    static constexpr i32 backspace_echo_length = 2;
    i32 backspace_clock = 0;
    bool backspace_first_echo = true;

    Label& label;
    Sprite& caret;
};