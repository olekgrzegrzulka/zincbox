#pragma once
#include <functional>
#include <utility>
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Label;
  class Root;

  class TextInput : public Sprite {
    public:
      TextInput(Root& ui_);
      void input() override;
      void update() override;
      void on_focused() override;
      void on_unfocused() override;
      void clear();
      void event(Input::InputEventMouseButton&) override;
      void event(Input::InputEventKey&) override;
      void set_on_text_changed(std::function<void()> lambda) { lambda_on_text_changed = std::move(lambda); }
      Label& label;
      ColorRect& caret;

    protected:
      std::function<void()> lambda_on_text_changed;
      float caret_blink_time = default_settings::text_input_caret_blink_time;
      float backspace_echo_length_initial = default_settings::text_input_backspace_echo_initial;
      float backspace_echo_length = default_settings::text_input_backspace_echo;
      float caret_blink_clock = default_settings::text_input_caret_blink_time;
      float backspace_clock = 0.0f;
      bool backspace_first_echo = true;
  };

} // namespace zincgui
