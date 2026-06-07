#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "common/debug.hpp"
#include "ui/popup_controller.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

PopupController::PopupController(UI& ui_) : Widget(ui_), ui(ui_), dimmer(add_child<Dimmer>()) {
  popups = &add_child<Widget>();
  dimmer.on_pressed = [this]() { on_dimmer_pressed(); };
  dimmer.on_enter_pressed = [this]() { on_dimmer_enter_pressed(); };
  dimmer.on_escape_pressed = [this]() { on_dimmer_escape_pressed(); };
}

void PopupController::on_dimmer_pressed() {
  for (auto [_, p] : popovers) {
    p->set_marked_for_deletion(true);
  }
  popovers.clear();
}

void PopupController::on_dimmer_enter_pressed() {}

void PopupController::on_dimmer_escape_pressed() {
  for (auto [_, p] : popovers) {
    p->set_marked_for_deletion(true);
  }
  popovers.clear();
}

void PopupController::create_popover(const popover_descriptor& d) {
  ensure(d.button_labels.size() >= d.button_actions.size());
  if (popovers.contains(d.id)) { return; }

  i32 space_needed = 8 + (4 + 24) * d.button_labels.size() + 12;
  bool bottom = ui.get_window_height() - d.at.y >= space_needed;

  auto& popover = add_child<Popover>();
  popovers[d.id] = &popover;

  popover.set_is_drawn_on_top(true);
  popover.set_nine_slice_margin(8.0f);
  popover.set_anchor(bottom ? Anchor::TOP : Anchor::BOTTOM);
  popover.set_layout("ttb fit expand fill m:4 s:0");
  if (bottom) {
    popover.get_layout().direction = LayoutDirection::TOP_TO_BOTTOM;
  } else {
    popover.get_layout().direction = LayoutDirection::BOTTOM_TO_TOP;
  }

  popover.set_width(50);

  auto* arrow = &popover.add_child<Sprite>(bottom ? "popover_arrow" : "popover_arrow_inverted");
  arrow->set_ignore_parents_layout(true);
  arrow->set_parent_anchor(bottom ? Anchor::TOP : Anchor::BOTTOM);
  arrow->set_anchor(bottom ? Anchor::BOTTOM : Anchor::TOP);
  arrow->set_y(bottom ? 1 : -1);
  arrow->set_is_drawn(d.show_arrow);
  if (bottom) {
    popover.set_pos(d.at.x, d.at.y + (d.distance + arrow->get_height()));
  } else {
    popover.set_pos(d.at.x, d.at.y - (d.distance + arrow->get_height()));
  }
  std::vector<Button*> buttons;
  for (auto& sv : d.button_labels) {
    auto& btn = popover.add_child<Button>(std::string(sv));
    buttons.emplace_back(&btn);
    btn.set_nine_slice_margin(8.0f);
    btn.set_texture_idle("button_popover_idle");
    btn.set_texture_hovered("button_popover_hovered");
    btn.set_texture_pressed("button_popover_pressed");
    btn.set_texture_disabled("button_popover_disabled");
    btn.set_texture("button_popover_idle", false);
    btn.set_min_height(22);
    btn.set_max_height(22);
    btn.set_height(22);
    popover.set_width(std::max(popover.get_width(), btn.get_label().get_width() + 30));
  }

  size_t button_i = 0;
  std::string popover_id = d.id;
  for (auto& l : d.button_actions) {
    auto lambda = [this, l, popover_id, &popover]() {
      if (l) { l(); }
      if (popovers.contains(popover_id)) { popovers[popover_id] = nullptr; }
      popover.set_marked_for_deletion(true);
    };
    buttons[button_i]->on_press(lambda);
    button_i += 1;
  }
};

bool PopupController::is_popup_open() const { return popups->get_children().size() > 0; }

void PopupController::close_all_popups() {
  for (auto [popup_id, popup] : popovers) {
    popup->set_marked_for_deletion(true);
  }
  popovers.clear();

  for (auto& popup : popups->get_children()) {
    popup->set_marked_for_deletion(true);
  }
}

void PopupController::input() {
  popups->input();
  for (auto& [_, w] : popovers) {
    if (!w) { continue; }
    w->input();
  }

  std::erase_if(popovers, [](const auto& item) { return item.second == nullptr; });

  if (dimmer.get_is_updated()) { dimmer.input(); }
}

void PopupController::update() {
  set_size(ui.get_window_size());
  popups->set_size(ui.get_window_size());
  bool dimmer_block_events = popovers.size() + popups->get_children().size() > 0;
  bool dimmer_visible = popups->get_children().size() > 0;
  dimmer.set_is_active(dimmer_block_events);
  dimmer.set_is_drawn(dimmer_visible);
  Widget::update();
}
