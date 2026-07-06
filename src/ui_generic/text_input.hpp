#pragma once
#include "common/input.hpp"
#include "label.hpp"
#include "sprite.hpp"
#include "ui_generic/widget.hpp"

class TextInput : public Sprite {
  public:
    TextInput(UI& ui_);
    void update() override;
    void clear();
    void event(Input::InputEventMouseButton&) override;
    void event(Input::InputEventKey&) override;
    void set_on_text_changed(std::function<void()> lambda) { lambda_on_text_changed = std::move(lambda); }
    Label& label;
    Sprite& caret;

  protected:
    std::function<void()> lambda_on_text_changed;
    static constexpr float caret_blink_time = 250.0f;
    static constexpr float backspace_echo_length_initial = 250.0f;
    static constexpr float backspace_echo_length = 19.0f;
    bool focused = false;
    float caret_blink_clock = caret_blink_time;
    float backspace_clock = 0.0f;
    bool backspace_first_echo = true;

  public:
    WIDGET_DEF_SETTER_DIRTY(focused)
    WIDGET_DEF_GETTER(focused)
};
