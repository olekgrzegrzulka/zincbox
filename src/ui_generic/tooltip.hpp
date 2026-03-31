#pragma once
#include <string>
#include "common/types.hpp"
#include "label.hpp"
#include "panel.hpp"
#include "ui.hpp"

enum class ToolTipPosition {
  LEFT,
  RIGHT,
  ABOVE,
  BELOW,
};

class ToolTip : public Panel {
  public:
    ToolTip(UI& ui_, std::string name, ToolTipPosition pos_ = ToolTipPosition::RIGHT, i32 distance_ = 16) : Panel(ui_, Panel::PanelStyle::Rounded) {
      pos = pos_;
      distance = distance_;
      set_ignore_parents_layout(true);
      set_is_drawn_on_top(true);

      label = &add_child<Label>(name);
      label->set_parent_anchor(Anchor::CENTER);
      label->set_anchor(Anchor::CENTER);

      set_size_and_position();
    }

    void set_text(const std::string& s) {
      label->set_text(s);
      label->set_size(vec2i{label->get_text_extents()});

      set_size_and_position();
    }

    void set_size_and_position() {
      label->set_size(vec2i{label->get_text_extents()});
      set_size(vec2i{label->get_text_extents()} + vec2i{20, 20});

      if (pos == ToolTipPosition::LEFT) {
        set_parent_anchor(Anchor::CENTER_LEFT);
        set_anchor(Anchor::CENTER_RIGHT);
        set_x(-distance);
      }
      if (pos == ToolTipPosition::RIGHT) {
        set_parent_anchor(Anchor::CENTER_RIGHT);
        set_anchor(Anchor::CENTER_LEFT);
        set_x(+distance);
      }
      if (pos == ToolTipPosition::BELOW) {
        set_parent_anchor(Anchor::BOTTOM_CENTER);
        set_anchor(Anchor::TOP_CENTER);
        set_y(+distance);
      }
      if (pos == ToolTipPosition::ABOVE) {
        set_parent_anchor(Anchor::TOP_CENTER);
        set_anchor(Anchor::BOTTOM_CENTER);
        set_y(-distance);
      }
    }

    void draw() override {
      i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
      i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
      i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
      i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());

      if (label->get_dirty()) { mark_dirty(); }

      set_x(get_x() + off_screen_left - off_screen_right);
      set_y(get_y() + off_screen_top - off_screen_bottom);

      Panel::draw();
    }

  protected:
    Label* label{};
    ToolTipPosition pos{};
    i32 distance{};
};
