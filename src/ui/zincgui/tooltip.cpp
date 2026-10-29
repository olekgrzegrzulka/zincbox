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
  if (is_layout_dirty()) { return; }

  if (clamp) {
    vec2i top_left = vec2i(x, y) - vec2i(vec2f{width, height} * anchor_to_uv(anchor));

    i32 off_screen_left = std::max(0, 0 - top_left.x);
    i32 off_screen_right = std::max(0, (top_left.x + width) - ui.get_window_width());
    i32 off_screen_top = std::max(0, 0 - top_left.y);
    i32 off_screen_bottom = std::max(0, (top_left.y + height) - ui.get_window_height());
    i32 push_x = off_screen_left - off_screen_right;
    i32 push_y = off_screen_top - off_screen_bottom;
    i32 lo = std::min((-width / 2) + 16, (width / 2) - 16);
    i32 hi = std::max((-width / 2) + 16, (width / 2) - 16);
    // arrow_offset = std::clamp(arrow_offset - push_x, lo, hi);
    // arrow->set_x(arrow_offset);
    set_x(get_x() + push_x);
    set_y(get_y() + push_y);
  }

  Sprite::update();
}
