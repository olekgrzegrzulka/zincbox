#pragma once
#include <functional>
#include <string>
#include "label.hpp"
#include "sprite.hpp"
#include "ui/theme.hpp" // FIXME ui_generic should not depend on ui/theme.hpp
#include "widget.hpp"

class UI;

enum class ButtonState : u8 { IDLE, HOVERED, PRESSED, DISABLED };

class Button : public Sprite {
  protected:
    Label& label;
    Sprite* image{};
    std::function<void()> lambda_press{};
    std::function<void()> lambda_press_rmb{};
    std::function<void()> lambda_depress{};
    ButtonState state = ButtonState::IDLE;
    bool switch_mode = false;
    bool is_switched = false;
    bool offset_label_on_press = false;

    bool mouse_hovering = false;
    bool mouse_pressed = false;
    bool rmb_held = false;

    vec2f uv_start_idle{};
    vec2f uv_end_idle{};
    vec2f uv_start_hovered{};
    vec2f uv_end_hovered{};
    vec2f uv_start_pressed{};
    vec2f uv_end_pressed{};
    vec2f uv_start_disabled{};
    vec2f uv_end_disabled{};

  public:
    Button(UI& ui_) : Sprite::Sprite(ui_), label(add_child<Label>()) { init(); }

    Button(UI& ui_, std::u32string_view label_) : Sprite::Sprite(ui_), label(add_child<Label>(label_)) { init(); }

    Button(UI& ui_, std::string_view label_) : Sprite::Sprite(ui_), label(add_child<Label>(label_)) { init(); }

    void init() {
      set_size(64, 24);
      set_texture_idle("button_idle");
      set_texture_hovered("button_hovered");
      set_texture_pressed("button_pressed");
      set_texture_disabled("button_disabled");
      set_nine_slice_margin(3.0f);
      set_nine_slice_scale(1.0f);

      get_label().set_text_color(theme::get_prop("text_color").as_rgba());

      set_sprite_idle();
    }

    Label& get_label() { return label; }
    const Label& get_label() const { return label; }

    virtual void update() override;

    virtual void draw() override { Sprite::draw(); }

    void on_press(std::function<void()> lambda_press_) { lambda_press = std::move(lambda_press_); }

    void on_depress(std::function<void()> lambda_depress_) { lambda_depress = std::move(lambda_depress_); }

    void on_press_rmb(std::function<void()> lambda_press_rmb_) { lambda_press_rmb = std::move(lambda_press_rmb_); }

    void add_image(std::string_view id, bool resize_to_texture_size = true);

    // Returns true if button's state was changed
    bool set_state(ButtonState state_) {
      if (state_ == state) { return false; }
      auto prev_state = state;
      state = state_;
      on_state_changed(prev_state);
      return true;
    }

    bool get_is_switched() const { return is_switched; }

    void set_is_switched(bool state_) {
      if (is_switched == state_) { return; }
      is_switched = state_;
      if (is_switched) {
        set_state(ButtonState::PRESSED);
        pressed();
      } else {
        set_state(ButtonState::HOVERED);
        depressed();
      }
    }

    void set_disabled(bool disabled) {
      if (disabled && state != ButtonState::DISABLED) {
        set_state(ButtonState::DISABLED);
      } else if (!disabled && state == ButtonState::DISABLED) {
        set_state(ButtonState::IDLE);
      }
    }

    WIDGET_DEF_GETTER(state);

    WIDGET_DEF_SETTER_DIRTY(switch_mode);
    WIDGET_DEF_GETTER(switch_mode);

    WIDGET_DEF_SETTER_DIRTY(offset_label_on_press);
    WIDGET_DEF_GETTER(offset_label_on_press);

    void set_uv_start_idle(vec2f to) {
      if (uv_start_idle == to) { return; }
      uv_start_idle = to;
      if (state == ButtonState::IDLE) { dirty = true; }
    }
    void set_uv_end_idle(vec2f to) {
      if (uv_end_idle == to) { return; }
      uv_end_idle = to;
      if (state == ButtonState::IDLE) { dirty = true; }
    }
    void set_uv_start_hovered(vec2f to) {
      if (uv_start_hovered == to) { return; }
      uv_start_hovered = to;
      if (state == ButtonState::HOVERED) { dirty = true; }
    }
    void set_uv_end_hovered(vec2f to) {
      if (uv_end_hovered == to) { return; }
      uv_end_hovered = to;
      if (state == ButtonState::HOVERED) { dirty = true; }
    }
    void set_uv_start_pressed(vec2f to) {
      if (uv_start_pressed == to) { return; }
      uv_start_pressed = to;
      if (state == ButtonState::PRESSED) { dirty = true; }
    }
    void set_uv_end_pressed(vec2f to) {
      if (uv_end_pressed == to) { return; }
      uv_end_pressed = to;
      if (state == ButtonState::PRESSED) { dirty = true; }
    }
    void set_uv_start_disabled(vec2f to) {
      if (uv_start_disabled == to) { return; }
      uv_start_disabled = to;
      if (state == ButtonState::DISABLED) { dirty = true; }
    }
    void set_uv_end_disabled(vec2f to) {
      if (uv_end_disabled == to) { return; }
      uv_end_disabled = to;
      if (state == ButtonState::DISABLED) { dirty = true; }
    }

    void set_texture_idle(std::string id);
    void set_texture_hovered(std::string id);
    void set_texture_pressed(std::string id);
    void set_texture_disabled(std::string id);

  protected:
    void set_sprite_idle() {
      set_uv_start(uv_start_idle);
      set_uv_end(uv_end_idle);
    }

    void set_sprite_hovered() {
      set_uv_start(uv_start_hovered);
      set_uv_end(uv_end_hovered);
    }

    void set_sprite_pressed() {
      set_uv_start(uv_start_pressed);
      set_uv_end(uv_end_pressed);
    }

    void set_sprite_disabled() {
      set_uv_start(uv_start_disabled);
      set_uv_end(uv_end_disabled);
    }

  public:
    void pressed() {
      if (lambda_press) { lambda_press(); }
    }

    void depressed() {
      if (lambda_depress) { lambda_depress(); }
    }

  protected:
    void on_state_changed(ButtonState /* prev_state */) {
      if (state == ButtonState::HOVERED) {
        set_sprite_hovered();
        get_label().set_text_color(theme::get_prop("text_color").as_rgba());
      } else if (state == ButtonState::IDLE) {
        set_sprite_idle();
        get_label().set_text_color(theme::get_prop("text_color").as_rgba());
      } else if (state == ButtonState::PRESSED) {
        set_sprite_pressed();
        get_label().set_text_color(theme::get_prop("text_color").as_rgba());
      } else if (state == ButtonState::DISABLED) {
        set_sprite_disabled();
        get_label().set_text_color(theme::get_prop("text_color_disabled").as_rgba());
      }
    }

    void event(Input::InputEventMouseButton&) override;

    void event(Input::InputEventMouseMove&) override;

    void event(Input::InputEventMouseScroll&) override;
};
