#pragma once
#include "common/input.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class Splitter final : public Sprite {
  public:
    Splitter(UI& ui_) : Sprite(ui_) {
      set_texture("splitter", false);
      set_width(6);
      set_nine_slice_margin(2.0f);
    }

    double get_ratio() const { return ratio; }
    void set_ratio(double r) { ratio = r; }

    void handle_event(Input::InputEventMouseMove&) override {
    }

    void process_input() override {
      bool lmb = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
      if (!is_mouse_hovering() && !lmb) { mouse_hovering = false; }

      if (mouse_hovering && lmb) {
        ratio = std::clamp(Input::get_mouse_x() / (double)ui.get_window_width(), 0.01, 0.99);
      }

      Sprite::process_input();
    }

    void handle_event(Input::InputEventMouseButton& ev) override {
      if (is_mouse_hovering() && ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT && ev.action == Input::MouseAction::PRESS) {
        mouse_hovering = true;
        ev.handled = true;
      }
    }

  private:
    double ratio = 0.5;
    bool mouse_hovering = false;
};
