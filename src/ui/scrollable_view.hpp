#pragma once
#include "common/types.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
  class Sprite;
  class ScrollBar;

  namespace Input {
    struct InputEventMouseScroll;
  }
} // namespace zincgui

class ScrollableView : public zincgui::Widget {
  public:
    ScrollableView(zincgui::Root& ui_);
    virtual void input() override;
    virtual void update() override;
    virtual void event(zincgui::Input::InputEventMouseScroll& ev) override;

    zincgui::Widget* content() { return m_content; }
    zincgui::Sprite* background() { return m_background; }
    zincgui::ScrollBar* scrollbar() { return m_scrollbar; }

  protected:
    i32 m_scroll_px = 0;
    i32 m_target_scroll_px = 0;
    zincgui::Sprite* m_background{};
    zincgui::Widget* m_container{};
    zincgui::Widget* m_content{};
    zincgui::ScrollBar* m_scrollbar{};
};
