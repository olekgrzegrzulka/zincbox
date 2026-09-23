#pragma once
#include "theme_config.hpp"
#include "ui/theme.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"

class Splitter final : public zincgui::ColorRect {
  public:
    Splitter(zincgui::Root& ui_) : ColorRect(ui_) {
      set_color(theme::config().splitter_color);
      set_width(6);
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

      ColorRect::input();
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
