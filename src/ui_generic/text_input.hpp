#pragma once
#include "label.hpp"
#include "sprite.hpp"

class TextInput : public Sprite {
  public:
    TextInput(UI& ui_);
    void update();
    void clear();

  protected:
    virtual void on_text_changed() {}

  protected:
    bool focused = false;

    static constexpr float caret_blink_time = 250.0f;
    float caret_blink_clock = caret_blink_time;

    static constexpr float backspace_echo_length_initial = 250.0f;
    static constexpr float backspace_echo_length = 15.0f;
    float backspace_clock = 0.0f;
    bool backspace_first_echo = true;

    Label& label;
    Sprite& caret;
};
