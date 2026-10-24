#pragma once
#include "ui/zincgui/color_rect.hpp"

namespace zincgui {
  class Root;
  namespace Input {
    class InputEventMouseMove;
    class InputEventMouseButton;
  } // namespace Input
} // namespace zincgui

class Splitter final : public zincgui::ColorRect {
  public:
    Splitter(zincgui::Root& ui_);

    void input() override;
    void event(zincgui::Input::InputEventMouseMove&) override;
    void event(zincgui::Input::InputEventMouseButton&) override;

    double get_ratio() const;
    void set_ratio(double r);
    bool get_is_dragged() const;

  private:
    double m_ratio = 0.5;
    bool m_is_dragged = false;
};
