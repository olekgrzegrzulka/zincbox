#pragma once
#include <functional>
#include <string>
#include <utility>
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
      void event(Input::InputEventMouseButton& ev) override;
      void event(Input::InputEventMouseScroll& ev) override;

      void on_text_changed();

      [[nodiscard]] i32 value() const noexcept { return m_value; }
      Spinner& value(i32 value_) {
        set_value_impl(value_);
        return *this;
      }

      [[nodiscard]] i32 min_value() const noexcept { return m_min_value; }
      Spinner& min_value(i32 min_value_) {
        set_min_value_impl(min_value_);
        return *this;
      }

      [[nodiscard]] i32 max_value() const noexcept { return m_max_value; }
      Spinner& max_value(i32 max_value_) {
        set_max_value_impl(max_value_);
        return *this;
      }

      [[nodiscard]] i32 button_step() const noexcept { return m_button_step; }
      Spinner& button_step(i32 v) noexcept {
        m_button_step = v;
        return *this;
      }

      [[nodiscard]] const std::string& postfix() const noexcept { return m_postfix; }
      Spinner& postfix(const std::string& postfix_) {
        set_postfix_impl(postfix_);
        return *this;
      }

      Spinner& on_value_changed(std::function<void()> lambda_value_changed_) {
        m_on_value_changed = std::move(lambda_value_changed_);
        return *this;
      }

      [[nodiscard]] Button& button_increase() noexcept { return m_button_increase; }
      [[nodiscard]] const Button& button_increase() const noexcept { return m_button_increase; }
      [[nodiscard]] Button& button_decrease() noexcept { return m_button_decrease; }
      [[nodiscard]] const Button& button_decrease() const noexcept { return m_button_decrease; }
      [[nodiscard]] Label& label_postfix() noexcept { return m_label_postfix; }
      [[nodiscard]] const Label& label_postfix() const noexcept { return m_label_postfix; }

    private:
      bool set_value_impl(i32);
      void set_postfix_impl(const std::string&);
      void set_min_value_impl(i32);
      void set_max_value_impl(i32);

    private:
      Button& m_button_increase;
      Button& m_button_decrease;
      Label& m_label_postfix;

      std::function<void()> m_on_value_changed = nullptr;

      i32 m_value = 0;
      i32 m_min_value = default_settings::spinner_min_value;
      i32 m_max_value = default_settings::spinner_max_value;
      bool m_buffer_changed = false;
      std::string m_postfix;
      i32 m_button_step = default_settings::spinner_button_step;
      i32 m_buttons_clock = 0;
      bool m_buttons_first_echo = true;
  };

} // namespace zincgui
