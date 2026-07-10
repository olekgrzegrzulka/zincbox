#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "ui/popup_controller.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

void PopupController::on_popup_closed(Popup* popup) {
  close_all_popovers();
  popup->set_marked_for_deletion(true);
  auto it = std::find(popups.begin(), popups.end(), popup);
  if (it != popups.end()) { popups.erase(it); }
}

PopupController::PopupController(UI& ui_) : Widget(ui_), ui(ui_) {
  dimmer = &add_child<Dimmer>();
  dimmer->on_pressed = [this]() { on_dimmer_pressed(); };
  dimmer->on_enter_pressed = [this]() { on_dimmer_enter_pressed(); };
  dimmer->on_escape_pressed = [this]() { on_dimmer_escape_pressed(); };
}

void PopupController::on_dimmer_pressed() { close_all_popovers(); }

void PopupController::on_dimmer_enter_pressed() {}

void PopupController::on_dimmer_escape_pressed() { close_all_popovers(); }

void PopupController::create_popover(const popover_descriptor& d) {
  i32 space_needed = 8 + (4 + 24) * d.buttons.size() + 12;
  bool arrow_on_top = ui.get_window_height() - d.at.y >= space_needed;

  auto& popover = add_child<Popover>(arrow_on_top);
  popovers.emplace(d.id, &popover);

  // popover.set_is_drawn_on_top(true);
  popover.set_nine_slice_margin(8.0f);
  popover.set_anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM);
  popover.set_layout("ttb fit expand fill m:4 s:0");
  if (arrow_on_top) {
    popover.get_layout().direction = LayoutDirection::TOP_TO_BOTTOM;
  } else {
    popover.get_layout().direction = LayoutDirection::BOTTOM_TO_TOP;
  }

  popover.set_width(50);
  popover.arrow->set_is_drawn(d.show_arrow);
  if (arrow_on_top) {
    popover.set_pos(d.at.x, d.at.y + (d.distance + popover.arrow->get_height()));
  } else {
    popover.set_pos(d.at.x, d.at.y - (d.distance + popover.arrow->get_height()));
  }
  std::vector<Button*> buttons;
  std::string popover_id = d.id;
  for (auto& [label, action] : d.buttons) {
    auto& btn = popover.add_child<Button>(label);
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

    auto lambda = [this, action, popover_id, &popover]() -> void {
      if (action) { action(); }
      popovers.erase(popover_id);
      popover.set_marked_for_deletion(true);
    };
    btn.on_press(lambda);
  }
};

bool PopupController::is_popup_open() const { return popups.size() > 0; }

void PopupController::close_all_popups() {
  for (auto [_, popover] : popovers) {
    popover->set_marked_for_deletion(true);
  }
  popovers.clear();

  for (auto popup : popups) {
    popup->set_marked_for_deletion(true);
  }
  popups.clear();
}

void PopupController::close_all_popovers() {
  for (auto [_, popover] : popovers) {
    popover->set_marked_for_deletion(true);
  }
  popovers.clear();
}

void PopupController::input() {
  if (children.size() >= 2) {
    children.back()->input();
    dimmer->input();
  }
}

void PopupController::update() {
  set_size(ui.get_window_size());
  bool dimmer_block_events = (popovers.size() + popups.size()) > 0;
  bool dimmer_visible = popups.size() > 0;
  dimmer->set_is_active(dimmer_block_events);
  dimmer->set_is_drawn(dimmer_visible);
  Widget::update();
}
