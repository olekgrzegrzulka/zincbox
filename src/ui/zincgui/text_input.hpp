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
      void event(Input::InputEventMouseButton& ev) override;
      void event(Input::InputEventKey& ev) override;

      void clear();

      [[nodiscard]] Label& label() noexcept { return m_label; }
      [[nodiscard]] const Label& label() const noexcept { return m_label; }
      [[nodiscard]] ColorRect& caret() noexcept { return m_caret; }
      [[nodiscard]] const ColorRect& caret() const noexcept { return m_caret; }
      [[nodiscard]] float caret_blink_time() const noexcept { return m_caret_blink_time; }
      TextInput& caret_blink_time(float v) noexcept {
        m_caret_blink_time = v;
        return *this;
      }

      TextInput& on_text_changed(std::function<void()> lambda) {
        m_on_text_changed = std::move(lambda);
        return *this;
      }

    protected:
      [[nodiscard]] float caret_blink_clock() const noexcept { return m_caret_blink_clock; }
      void caret_blink_clock(float v) noexcept { m_caret_blink_clock = v; }

    private:
      Label& m_label;
      ColorRect& m_caret;
      std::function<void()> m_on_text_changed;
      float m_caret_blink_time = default_settings::text_input_caret_blink_time;
      float m_backspace_echo_length_initial = default_settings::text_input_backspace_echo_initial;
      float m_backspace_echo_length = default_settings::text_input_backspace_echo;
      float m_caret_blink_clock = default_settings::text_input_caret_blink_time;
      float m_backspace_clock = 0.0f;
      bool m_backspace_first_echo = true;
  };

} // namespace zincgui
