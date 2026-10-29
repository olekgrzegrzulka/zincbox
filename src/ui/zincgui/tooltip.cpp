#include "tooltip.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"

using namespace zincgui;

ToolTip::ToolTip(Root& ui_, std::string_view name_, ToolTipPosition pos_, i32 distance_) :
    Sprite(ui_, default_settings::tooltip_texture) {
  m_pos = pos_;
  m_distance = distance_;
  ignore_parents_layout(true)
    .is_drawn_on_top(true)
    .nine_slice_margin(default_settings::tooltip_nine_slice_margin);

  m_label = &add_child<Label>(name_);
  m_label->parent_anchor(Anchor::CENTER)
    .anchor(Anchor::CENTER);

  set_size_and_position();
}

void ToolTip::update() {
  if (is_layout_dirty()) { return; }

  if (m_clamp) {
    vec2i top_left = vec2i(x(), y()) - vec2i(vec2f{static_cast<float>(width()), static_cast<float>(height())} * anchor_to_uv(anchor()));

    i32 off_screen_left = std::max(0, 0 - top_left.x);
    i32 off_screen_right = std::max(0, (top_left.x + width()) - ui().get_window_width());
    i32 off_screen_top = std::max(0, 0 - top_left.y);
    i32 off_screen_bottom = std::max(0, (top_left.y + height()) - ui().get_window_height());
    i32 push_x = off_screen_left - off_screen_right;
    i32 push_y = off_screen_top - off_screen_bottom;
    i32 lo = std::min((-width() / 2) + 16, (width() / 2) - 16);
    i32 hi = std::max((-width() / 2) + 16, (width() / 2) - 16);
    // arrow_offset = std::clamp(arrow_offset - push_x, lo, hi);
    // arrow->set_x(arrow_offset);
    x(x() + push_x)
      .y(y() + push_y);
  }

  Sprite::update();
}

void ToolTip::set_size_and_position() {
  m_label->size(vec2i{m_label->text_extents()});
  size(vec2i{m_label->text_extents()} + default_settings::tooltip_padding);

  if (m_pos == ToolTipPosition::LEFT) {
    parent_anchor(Anchor::CENTER_LEFT)
      .anchor(Anchor::CENTER_RIGHT)
      .x(-m_distance);
  }
  if (m_pos == ToolTipPosition::RIGHT) {
    parent_anchor(Anchor::CENTER_RIGHT)
      .anchor(Anchor::CENTER_LEFT)
      .x(+m_distance);
  }
  if (m_pos == ToolTipPosition::BELOW) {
    parent_anchor(Anchor::BOTTOM_CENTER)
      .anchor(Anchor::TOP_CENTER)
      .y(+m_distance);
  }
  if (m_pos == ToolTipPosition::ABOVE) {
    parent_anchor(Anchor::TOP_CENTER)
      .anchor(Anchor::BOTTOM_CENTER)
      .y(-m_distance);
  }
}

ToolTip& ToolTip::text(const std::string& s) {
  m_label->text(s);
  set_size_and_position();
  return *this;
}

ToolTip& ToolTip::clamp(bool value) {
  if (m_clamp != value) { m_clamp = value; }
  return *this;
}
