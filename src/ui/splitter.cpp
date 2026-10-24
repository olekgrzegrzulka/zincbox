#include "splitter.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/widget.hpp"

Splitter::Splitter(zincgui::Root& ui_) : ColorRect(ui_) {
  set_color(theme::config().splitter_color);
  set_width(6);
}

double Splitter::get_ratio() const { return m_ratio; }

void Splitter::set_ratio(double r) { m_ratio = r; }

void Splitter::event(zincgui::Input::InputEventMouseMove&) {}

void Splitter::input() {
  bool lmb = zincgui::Input::mouse_pressed(zincgui::Input::MouseButton::MOUSE_BUTTON_LEFT);
  if (!is_mouse_hovering() && !lmb) { m_is_dragged = false; }

  if (m_is_dragged && lmb) {
    m_ratio = std::clamp(zincgui::Input::get_mouse_x() / (double)ui.get_window_width(), 0.01, 0.99);
  }

  ColorRect::input();
}

void Splitter::event(zincgui::Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering() && ev.button == zincgui::Input::MouseButton::MOUSE_BUTTON_LEFT &&
      ev.action == zincgui::Input::MouseAction::PRESS) {
    m_is_dragged = true;
    ev.handled = true;
  }
}

bool Splitter::get_is_dragged() const { return m_is_dragged; }
