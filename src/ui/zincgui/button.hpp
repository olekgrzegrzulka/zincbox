#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include "common/types.hpp"
#include "ui/zincgui/sprite.hpp"

namespace zincgui {

  class Root;
  class Label;
  class Sprite;

  enum class ButtonState : u8 { IDLE, HOVERED, PRESSED, DISABLED };

  class Button : public Sprite {
    public:
      Button(Root& ui_);
      Button(Root& ui_, std::string_view label_);
      ~Button() override = default;

      void update() override;
      void event(Input::InputEventMouseButton&) override;
      void event(Input::InputEventMouseMove&) override;
      void event(Input::InputEventMouseScroll&) override;

      template <typename Self>
      Self& add_image(this Self& self, std::string_view id, bool resize_to_texture_size = true) {
        self.add_image_impl(id, resize_to_texture_size);
        return self;
      }
      void pressed();
      void depressed();

      [[nodiscard]] Label& label() noexcept { return m_label; }
      [[nodiscard]] const Label& label() const noexcept { return m_label; }

      [[nodiscard]] Sprite* image() noexcept { return m_image; }
      [[nodiscard]] const Sprite* image() const noexcept { return m_image; }

      template <typename Self> Self& on_press(this Self& self, std::function<void()> lambda_press_) {
        self.m_on_press = std::move(lambda_press_);
        return self;
      }

      template <typename Self> Self& on_depress(this Self& self, std::function<void()> lambda_depress_) {
        self.m_on_depress = std::move(lambda_depress_);
        return self;
      }

      template <typename Self> Self& on_press_rmb(this Self& self, std::function<void()> lambda_press_rmb_) {
        self.m_on_press_rmb = std::move(lambda_press_rmb_);
        return self;
      }

      [[nodiscard]] ButtonState state() const noexcept { return m_state; }
      template <typename Self> Self& state(this Self& self, ButtonState state_) {
        if (state_ == self.m_state) { return self; }
        auto prev_state = self.m_state;
        self.m_state = state_;
        self.on_state_changed(prev_state);
        return self;
      }

      [[nodiscard]] bool is_switched() const noexcept { return m_is_switched; }
      template <typename Self> Self& is_switched(this Self& self, bool state_) {
        if (self.m_is_switched == state_) { return self; }
        self.m_is_switched = state_;
        if (self.m_is_switched) {
          self.state(ButtonState::PRESSED);
          self.pressed();
        } else {
          self.state(ButtonState::HOVERED);
          self.depressed();
        }
        return self;
      }

      [[nodiscard]] bool switch_mode() const noexcept { return m_switch_mode; }
      template <typename Self> Self& switch_mode(this Self& self, bool switch_mode_) {
        if (self.m_switch_mode != switch_mode_) {
          self.m_switch_mode = switch_mode_;
          self.mark_dirty();
        }
        return self;
      }

      template <typename Self> Self& disabled(this Self& self, bool disabled_) {
        if (disabled_ && self.m_state != ButtonState::DISABLED) {
          self.state(ButtonState::DISABLED);
        } else if (!disabled_ && self.m_state == ButtonState::DISABLED) {
          self.state(ButtonState::IDLE);
        }
        return self;
      }

      [[nodiscard]] vec2f uv_start_idle() const noexcept { return m_uv_start_idle; }
      template <typename Self> Self& uv_start_idle(this Self& self, vec2f to) {
        if (self.m_uv_start_idle == to) { return self; }
        self.m_uv_start_idle = to;
        if (self.m_state == ButtonState::IDLE) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_end_idle() const noexcept { return m_uv_end_idle; }
      template <typename Self> Self& uv_end_idle(this Self& self, vec2f to) {
        if (self.m_uv_end_idle == to) { return self; }
        self.m_uv_end_idle = to;
        if (self.m_state == ButtonState::IDLE) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_start_hovered() const noexcept { return m_uv_start_hovered; }
      template <typename Self> Self& uv_start_hovered(this Self& self, vec2f to) {
        if (self.m_uv_start_hovered == to) { return self; }
        self.m_uv_start_hovered = to;
        if (self.m_state == ButtonState::HOVERED) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_end_hovered() const noexcept { return m_uv_end_hovered; }
      template <typename Self> Self& uv_end_hovered(this Self& self, vec2f to) {
        if (self.m_uv_end_hovered == to) { return self; }
        self.m_uv_end_hovered = to;
        if (self.m_state == ButtonState::HOVERED) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_start_pressed() const noexcept { return m_uv_start_pressed; }
      template <typename Self> Self& uv_start_pressed(this Self& self, vec2f to) {
        if (self.m_uv_start_pressed == to) { return self; }
        self.m_uv_start_pressed = to;
        if (self.m_state == ButtonState::PRESSED) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_end_pressed() const noexcept { return m_uv_end_pressed; }
      template <typename Self> Self& uv_end_pressed(this Self& self, vec2f to) {
        if (self.m_uv_end_pressed == to) { return self; }
        self.m_uv_end_pressed = to;
        if (self.m_state == ButtonState::PRESSED) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_start_disabled() const noexcept { return m_uv_start_disabled; }
      template <typename Self> Self& uv_start_disabled(this Self& self, vec2f to) {
        if (self.m_uv_start_disabled == to) { return self; }
        self.m_uv_start_disabled = to;
        if (self.m_state == ButtonState::DISABLED) { self.mark_dirty(); }
        return self;
      }

      [[nodiscard]] vec2f uv_end_disabled() const noexcept { return m_uv_end_disabled; }
      template <typename Self> Self& uv_end_disabled(this Self& self, vec2f to) {
        if (self.m_uv_end_disabled == to) { return self; }
        self.m_uv_end_disabled = to;
        if (self.m_state == ButtonState::DISABLED) { self.mark_dirty(); }
        return self;
      }

      template <typename Self> Self& texture_idle(this Self& self, std::string id) {
        self.set_texture_idle_impl(std::move(id));
        return self;
      }

      template <typename Self> Self& texture_hovered(this Self& self, std::string id) {
        self.set_texture_hovered_impl(std::move(id));
        return self;
      }

      template <typename Self> Self& texture_pressed(this Self& self, std::string id) {
        self.set_texture_pressed_impl(std::move(id));
        return self;
      }

      template <typename Self> Self& texture_disabled(this Self& self, std::string id) {
        self.set_texture_disabled_impl(std::move(id));
        return self;
      }

    protected:
      void sprite_idle() { uv_start(m_uv_start_idle).uv_end(m_uv_end_idle); }
      void sprite_hovered() { uv_start(m_uv_start_hovered).uv_end(m_uv_end_hovered); }
      void sprite_pressed() { uv_start(m_uv_start_pressed).uv_end(m_uv_end_pressed); }
      void sprite_disabled() { uv_start(m_uv_start_disabled).uv_end(m_uv_end_disabled); }
      virtual void on_state_changed(ButtonState /* prev_state */);

    private:
      void init();
      void add_image_impl(std::string_view id, bool resize_to_texture_size);
      void set_texture_idle_impl(std::string id);
      void set_texture_hovered_impl(std::string id);
      void set_texture_pressed_impl(std::string id);
      void set_texture_disabled_impl(std::string id);

    private:
      Label& m_label;
      Sprite* m_image{};
      std::function<void()> m_on_press;
      std::function<void()> m_on_press_rmb;
      std::function<void()> m_on_depress;
      ButtonState m_state = ButtonState::IDLE;
      bool m_switch_mode = false;
      bool m_is_switched = false;
      bool m_mouse_pressed = false;
      bool m_rmb_held = false;
      vec2f m_uv_start_idle{};
      vec2f m_uv_end_idle{};
      vec2f m_uv_start_hovered{};
      vec2f m_uv_end_hovered{};
      vec2f m_uv_start_pressed{};
      vec2f m_uv_end_pressed{};
      vec2f m_uv_start_disabled{};
      vec2f m_uv_end_disabled{};
  };

} // namespace zincgui
