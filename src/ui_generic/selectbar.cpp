#include "selectbar.hpp"
#include <algorithm>
#include <cmath>
#include "widget.hpp"

SelectBar::SelectBar(UI& ui_, Orientation orientation) : Sprite(ui_), highlight(add_child<Sprite>()) {
  set_texture("selectbar_bg");
  set_nine_slice_margin(4);

  highlight.set_texture("selectbar_selected");
  highlight.set_nine_slice_margin(4);
  highlight.set_ignore_parents_layout(true);
  highlight.set_is_drawn(false);

  auto dir = (orientation == Orientation::HORIZONTAL) ? LayoutDirection::LEFT_TO_RIGHT : LayoutDirection::TOP_TO_BOTTOM;

  bool fill = (orientation == Orientation::HORIZONTAL) ? true : false;

  get_layout() = {
    .enabled = true,
    .fit_to_contents = false,
    .expand_children = true,
    .fill = fill,
    .margin = 4,
    .spacing = 4,
    .direction = dir,
  };
}

Button& SelectBar::add_option(std::string label) {
  auto& btn = add_child<Button>(label);
  btn.set_height(24);

  options.push_back(&btn);
  i32 index = options.size() - 1;

  btn.on_press([this, index]() { select(index); });

  if (options.size() == 1) {
    select(0);
    highlight.set_is_drawn(true);
    anim_progress = 1.0f;
  }

  return btn;
}

void SelectBar::select(i32 index, bool animation) {
  if (index < 0 || index >= (i32)options.size()) { return; }

  if (selected_index != index) {
    old_x = (float)highlight.get_x();
    old_y = (float)highlight.get_y();
    old_w = (float)highlight.get_width();
    old_h = (float)highlight.get_height();

    anim_progress = animation ? 0.0f : 1.0f;

    selected_index = index;
    if (lambda_change) { lambda_change(selected_index); }
  }
}

void SelectBar::update() {
  Sprite::update();

  if (options.empty()) { return; }

  if (anim_progress < 1.0f) { anim_progress = std::min(1.0f, anim_progress + 0.25f); }

  for (i32 i = 0; i < (i32)options.size(); i += 1) {
    options[i]->set_is_self_drawn(i != selected_index && anim_progress >= 1.0f);
  }

  auto* target_btn = options[std::clamp(selected_index, 0, (i32)options.size() - 1)];

  bool orientation_horizontal = get_layout().direction == LayoutDirection::LEFT_TO_RIGHT;

  float target_x = (float)target_btn->get_x() + (orientation_horizontal ? 0.0f : 4.0f);
  float target_y = (float)target_btn->get_y() + (orientation_horizontal ? 4.0f : 0.0f);
  float target_w = (float)target_btn->get_width();
  float target_h = (float)target_btn->get_height();

  float progess_smooth = (anim_progress - 1);
  progess_smooth = 1 - progess_smooth * progess_smooth;
  highlight.set_x((i32)std::lerp(old_x, target_x, progess_smooth));
  highlight.set_y((i32)std::lerp(old_y, target_y, progess_smooth));
  highlight.set_width((i32)std::lerp(old_w, target_w, progess_smooth));
  highlight.set_height((i32)std::lerp(old_h, target_h, progess_smooth));
}