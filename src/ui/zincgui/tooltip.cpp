#include "tooltip.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"

using namespace zincgui;

ToolTip::ToolTip(Root& ui_) : Sprite(ui_, default_settings::tooltip_texture) {
  ignore_parents_layout(true)
    .is_drawn_on_top(true)
    .hit_test_behavior(HitTestBehavior::IGNORE)
    .parent_anchor(Anchor::TOP_LEFT)
    .anchor(Anchor::TOP)
    .nine_slice_margin(default_settings::tooltip_nine_slice_margin);

  m_arrow = &add_child<Sprite>("tooltip_arrow");
  m_arrow->ignore_parents_layout(true)
    .hit_test_behavior(HitTestBehavior::IGNORE)
    .parent_anchor(Anchor::TOP)
    .anchor(Anchor::BOTTOM)
    .y(2);

  m_label = &add_child<Label>("");
  m_label->ignore_parents_layout(true)
    .hit_test_behavior(HitTestBehavior::IGNORE)
    .parent_anchor(Anchor::CENTER)
    .anchor(Anchor::CENTER);
}

void ToolTip::update() {
  if (is_layout_dirty()) { return; }

  update_placement();
  Sprite::update();
}

void ToolTip::apply(const TooltipDescriptor& d) {
  m_pos = d.pos;
  m_target_pos = d.at;
  m_distance = d.distance;
  m_show_arrow = d.show_arrow;
  m_clamp = d.clamp;
  text(d.text);
  update_placement();
}

void ToolTip::update_placement() {
  m_label->size(vec2i{m_label->text_extents()});
  size(vec2i{m_label->text_extents()} + default_settings::tooltip_padding);

  bool arrow_on_top = (m_pos == ToolTipPosition::BELOW);
  parent_anchor(Anchor::TOP_LEFT).anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM);

  m_arrow->is_drawn(m_show_arrow);
  if (m_show_arrow) {
    m_arrow->texture(arrow_on_top ? "tooltip_arrow" : "tooltip_arrow_inverted", true)
      .parent_anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM)
      .anchor(arrow_on_top ? Anchor::BOTTOM : Anchor::TOP)
      .y(arrow_on_top ? 2 : -2);
  }

  i32 arrow_h = m_show_arrow ? m_arrow->height() : 0;
  i32 total_offset_y = m_distance + arrow_h;
  i32 ideal_x = m_target_pos.x;
  i32 ideal_y = arrow_on_top ? (m_target_pos.y + total_offset_y) : (m_target_pos.y - total_offset_y);

  i32 push_x = 0;
  i32 push_y = 0;

  if (m_clamp) {
    i32 top_left_x = ideal_x - (width() / 2);
    i32 top_left_y = arrow_on_top ? ideal_y : (ideal_y - height());

    i32 off_screen_left = std::max(0, 0 - top_left_x);
    i32 off_screen_right = std::max(0, (top_left_x + width()) - ui().get_window_width());
    i32 off_screen_top = std::max(0, 0 - top_left_y);
    i32 off_screen_bottom = std::max(0, (top_left_y + height()) - ui().get_window_height());

    push_x = off_screen_left - off_screen_right;
    push_y = off_screen_top - off_screen_bottom;
  }

  x(ideal_x + push_x).y(ideal_y + push_y);

  if (m_show_arrow) {
    i32 lo = std::min((-width() / 2) + 12, (width() / 2) - 12);
    i32 hi = std::max((-width() / 2) + 12, (width() / 2) - 12);
    m_arrow_offset = std::clamp(-push_x, lo, hi);
    m_arrow->x(m_arrow_offset);
  }
}

ToolTip& ToolTip::text(std::string_view s) {
  if (m_label->text() != s) { m_label->text(std::string(s)); }
  return *this;
}
