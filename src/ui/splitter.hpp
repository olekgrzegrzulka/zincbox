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
    void event(zincgui::Input::InputEventMouseMove& ev) override;
    void event(zincgui::Input::InputEventMouseButton& ev) override;

    [[nodiscard]] double ratio() const noexcept { return m_ratio; }
    Splitter& ratio(double r) noexcept {
      m_ratio = r;
      return *this;
    }

    [[nodiscard]] bool is_dragged() const noexcept { return m_is_dragged; }
    Splitter& is_dragged(bool d) noexcept {
      m_is_dragged = d;
      return *this;
    }

  private:
    double m_ratio = 0.5;
    bool m_is_dragged = false;
};
