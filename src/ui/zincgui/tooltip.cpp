#include "tooltip.hpp"
#include <string>
#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"

using namespace zincgui;

ToolTip::ToolTip(Root& ui_, std::string_view name_, ToolTipPosition pos_, i32 distance_)
  : Sprite(ui_, default_settings::tooltip_texture) {
  pos = pos_;
  distance = distance_;
  set_ignore_parents_layout(true);
  set_is_drawn_on_top(true);
  set_nine_slice_margin(default_settings::tooltip_nine_slice_margin);

  label = &add_child<Label>(name_);
  label->set_parent_anchor(Anchor::CENTER);
  label->set_anchor(Anchor::CENTER);

  set_size_and_position();
}

void ToolTip::set_text(const std::string& s) {
  label->set_text(s);

  set_size_and_position();
}

void ToolTip::set_clamp(bool value) {
  if (clamp != value) { clamp = value; }
}

void ToolTip::set_size_and_position() {
  label->set_size(vec2i{label->get_text_extents()});
  set_size(vec2i{label->get_text_extents()} + default_settings::tooltip_padding);

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

void ToolTip::update() {
  if (clamp) {
    i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
    i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
    i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
    i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());

    set_x(get_x() + off_screen_left - off_screen_right);
    set_y(get_y() + off_screen_top - off_screen_bottom);
  }

  Sprite::update();
}
