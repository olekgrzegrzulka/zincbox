#pragma once
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"

class Splitter final : public zincgui::Sprite {
  public:
    Splitter(zincgui::Root& ui_) : Sprite(ui_) {
      set_texture("splitter", false);
      set_width(6);
      set_nine_slice_margin(2.0f);
    }

    double get_ratio() const { return ratio; }
    void set_ratio(double r) { ratio = r; }

    void event(zincgui::Input::InputEventMouseMove&) override {}

    void input() override {
      bool lmb = zincgui::Input::mouse_pressed(zincgui::Input::MouseButton::MOUSE_BUTTON_LEFT);
      if (!is_mouse_hovering() && !lmb) { is_dragged = false; }

      if (is_dragged && lmb) {
        ratio = std::clamp(zincgui::Input::get_mouse_x() / (double)ui.get_window_width(), 0.01, 0.99);
      }

      Sprite::input();
    }

    void event(zincgui::Input::InputEventMouseButton& ev) override {
      if (is_mouse_hovering() && ev.button == zincgui::Input::MouseButton::MOUSE_BUTTON_LEFT &&
          ev.action == zincgui::Input::MouseAction::PRESS) {
        is_dragged = true;
        ev.handled = true;
      }
    }

    bool get_is_dragged() const { return is_dragged; }

  private:
    double ratio = 0.5;
    bool is_dragged = false;
};
