#include "ui/scrollable_view.hpp"
#include <algorithm>
#include <cmath>
#include "common/types.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

ScrollableView::ScrollableView(Root& ui_) : Widget(ui_) {
  m_background = &add_child<Sprite>();
  m_container = &add_child<Widget>();
  m_scrollbar = &add_child<ScrollBar>();

  hit_test_behavior(zincgui::HitTestBehavior::BLOCK)
    .layout("ltr expand fill");

  m_scrollbar->anchor(Anchor::RIGHT)
    .parent_anchor(Anchor::RIGHT);
  m_scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

  m_container->clip_children(true);

  m_content = &m_container->add_child<Widget>();
  m_content->layout("ttb m:8 s:8 left fit expand");

  m_background->texture("panel_dark")
    .ignore_parents_layout(true);
}

void ScrollableView::input() {}

void ScrollableView::update() {
  m_scrollbar->page_size(height())
    .content_size(m_content->height());

  double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
  m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);

  if (m_content->y() != -m_scroll_px || m_content->width() != m_container->width()) {
    m_content->width(m_container->width())
      .y(-m_scroll_px);
  }

  m_background->size(width(), height());

  Widget::update();
}

void ScrollableView::event(Input::InputEventMouseScroll& ev) {
  if (is_mouse_hovering()) {
    m_scrollbar->scroll(ev.offset.y);
    ev.handled = true;
  }
  if (!ev.handled) { Widget::event(ev); }
}

ScrollableView& ScrollableView::scroll_offset(i32 scroll, bool instant) {
  m_scrollbar->scroll_offset(scroll);
  m_target_scroll_px = m_scrollbar->scroll_offset();
  if (instant) { m_scroll_px = scroll; }
  return *this;
}
