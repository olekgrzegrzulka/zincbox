#pragma once
#include "common/types.hpp"
#include "ui_generic/widget.hpp"

class UI;
class Sprite;
class ScrollBar;
namespace Input {
  struct InputEventMouseScroll;
}

class ScrollableView : public Widget {
  public:
    ScrollableView(UI& ui_);
    virtual void input() override;
    virtual void update() override;
    virtual void event(Input::InputEventMouseScroll& ev) override;

    Widget* content() { return m_content; }
    Sprite* background() { return m_background; }
    ScrollBar* scrollbar() { return m_scrollbar; }

  protected:
    i32 m_scroll_px = 0;
    i32 m_target_scroll_px = 0;
    Sprite* m_background{};
    Widget* m_container{};
    Widget* m_content{};
    ScrollBar* m_scrollbar{};
};
