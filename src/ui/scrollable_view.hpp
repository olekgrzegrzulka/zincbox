#pragma once
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class ScrollableView : public Widget {
  public:
    ScrollableView(UI& ui_) : Widget(ui_) {
      m_background = &add_child<Sprite>();
      m_container = &add_child<Widget>();
      m_scrollbar = &add_child<ScrollBar>();

      set_layout("ltr expand fill");

      m_scrollbar->set_anchor(Anchor::RIGHT);
      m_scrollbar->set_parent_anchor(Anchor::RIGHT);
      m_scrollbar->set_width(10);
      m_scrollbar->set_min_width(10);
      m_scrollbar->set_max_width(10);
      m_scrollbar->set_thumb_thickness(10);
      m_scrollbar->set_orientation(SliderOrientation::VERTICAL);
      m_scrollbar->set_track_thickness(10);
      m_scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

      m_container->set_clip_children(true);

      m_content = &m_container->add_child<Widget>();
      m_content->set_layout("ttb m:8 s:8 left fit");

      m_background->set_ignore_parents_layout(true);
    }

    virtual void update() override {
      m_scrollbar->set_page_size(height);
      m_scrollbar->set_content_size(m_content->get_height());

      double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
      m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);

      if (m_content->get_y() != -m_scroll_px || m_content->get_width() != m_container->get_width()) {
        m_content->set_width(m_container->get_width());
        m_content->set_y(-m_scroll_px);
        ui.mark_dirty_recursive(m_content);
      }

      m_background->set_size(width, height);

      Widget::update();
    }

    virtual void event(Input::InputEventMouseScroll& ev) override {
      if (is_mouse_hovering()) {
        m_scrollbar->scroll(ev.offset.y);
        ev.handled = true;
      }
      if (!ev.handled) { Widget::event(ev); }
    }

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
