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

    void input() override;
    void update() override;
    void event(zincgui::Input::InputEventMouseScroll& ev) override;

    ScrollableView& scroll_offset(i32 scroll, bool instant = false);

    [[nodiscard]] zincgui::Widget* content() noexcept { return m_content; }
    [[nodiscard]] const zincgui::Widget* content() const noexcept { return m_content; }
    [[nodiscard]] zincgui::Widget* container() noexcept { return m_container; }
    [[nodiscard]] const zincgui::Widget* container() const noexcept { return m_container; }
    [[nodiscard]] zincgui::Sprite* background() noexcept { return m_background; }
    [[nodiscard]] const zincgui::Sprite* background() const noexcept { return m_background; }
    [[nodiscard]] zincgui::ScrollBar* scrollbar() noexcept { return m_scrollbar; }
    [[nodiscard]] const zincgui::ScrollBar* scrollbar() const noexcept { return m_scrollbar; }
    [[nodiscard]] i32 scroll_px() const noexcept { return m_scroll_px; }
    [[nodiscard]] i32 target_scroll_px() const noexcept { return m_target_scroll_px; }

  private:
    i32 m_scroll_px = 0;
    i32 m_target_scroll_px = 0;
    zincgui::Sprite* m_background{};
    zincgui::Widget* m_container{};
    zincgui::Widget* m_content{};
    zincgui::ScrollBar* m_scrollbar{};
};
