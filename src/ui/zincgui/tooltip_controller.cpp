#include "tooltip_controller.hpp"
#include "ui/zincgui/root.hpp"

using namespace zincgui;

TooltipController::TooltipController(Root& ui_) : Widget(ui_) {
  ignore_parents_layout(true)
    .is_drawn_on_top(true)
    .hit_test_behavior(HitTestBehavior::IGNORE)
    .parent_anchor(Anchor::TOP_LEFT)
    .anchor(Anchor::TOP_LEFT);

  m_tooltip = &add_child<ToolTip>();
  m_tooltip->is_drawn(false);
}

void TooltipController::update() {
  size(ui().get_window_size());

  if (!m_enabled || !m_requested_this_frame) {
    m_tooltip->is_drawn(false);
  } else {
    m_tooltip->is_drawn(true);
    m_tooltip->update_placement();
  }

  m_requested_this_frame = false;
  Widget::update();
}

TooltipController& TooltipController::show(const TooltipDescriptor& d) {
  if (!m_enabled || d.text.empty()) { return *this; }

  m_requested_this_frame = true;
  m_tooltip->apply(d);
  m_tooltip->is_drawn(true);
  return *this;
}

TooltipController& TooltipController::show(vec2i at, std::string_view text, ToolTipPosition pos, i32 distance,
                                           bool show_arrow, bool clamp) {
  return show(TooltipDescriptor{
    .text = text,
    .at = at,
    .pos = pos,
    .distance = distance,
    .show_arrow = show_arrow,
    .clamp = clamp,
  });
}

TooltipController& TooltipController::show(const Widget& target, std::string_view text, ToolTipPosition pos,
                                           i32 distance, bool show_arrow, bool clamp) {
  Anchor target_anchor = (pos == ToolTipPosition::BELOW) ? Anchor::BOTTOM_CENTER : Anchor::TOP_CENTER;
  return show(TooltipDescriptor{
    .text = text,
    .at = target.position(target_anchor),
    .pos = pos,
    .distance = distance,
    .show_arrow = show_arrow,
    .clamp = clamp,
  });
}

TooltipController& TooltipController::hide() {
  m_requested_this_frame = false;
  m_tooltip->is_drawn(false);
  return *this;
}
