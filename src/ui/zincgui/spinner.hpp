#pragma once
#include <functional>
#include <string>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/text_input.hpp"

namespace zincgui {
  class Root;
  class Widget;
  class Button;
  class Sprite;
  class Label;

  class Spinner : public TextInput {
    public:
      Spinner(Root& ui_);

      void update() override;
      void event(Input::InputEventMouseButton&) override;
      void event(Input::InputEventMouseScroll&) override;

      void on_text_changed();

    protected:
      Button& button_increase;
      Button& button_decrease;
      Label& label_postfix;

      std::function<void()> lambda_value_changed = nullptr;

      i32 value = 0;
      i32 min_value = default_settings::spinner_min_value;
      i32 max_value = default_settings::spinner_max_value;
      bool buffer_changed = false;
      std::string postfix;
      i32 button_step = default_settings::spinner_button_step;
      i32 buttons_clock = 0;
      bool buttons_first_echo = true;

    public:
      WIDGET_DEF_SETTER(button_step);

      WIDGET_DEF_GETTER(value);
      WIDGET_DEF_GETTER(min_value);
      WIDGET_DEF_GETTER(max_value);
      WIDGET_DEF_GETTER(button_step);

      void set_postfix(const std::string& postfix_);
      bool set_value(i32 value_);
      void on_value_changed(std::function<void()> lambda_value_changed_);
      void set_min_value(i32 min_value_);
      void set_max_value(i32 max_value_);
  };

} // namespace zincgui
