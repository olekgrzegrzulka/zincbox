#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "common/color.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

void PopupController::on_popup_closed(Popup* popup) {
  close_all_popovers();
  popup->set_marked_for_deletion(true);
  auto it = std::find(popups.begin(), popups.end(), popup);
  if (it != popups.end()) { popups.erase(it); }
}

PopupController::PopupController(Root& ui_) : Widget(ui_), ui(ui_) {
  dimmer = &add_child<Dimmer>();
  dimmer->on_pressed = [this]() { on_dimmer_pressed(); };
  dimmer->on_enter_pressed = [this]() { on_dimmer_enter_pressed(); };
  dimmer->on_escape_pressed = [this]() { on_dimmer_escape_pressed(); };
}

void PopupController::on_dimmer_pressed() { close_all_popovers(); }

void PopupController::on_dimmer_enter_pressed() {}

void PopupController::on_dimmer_escape_pressed() { close_all_popovers(); }

void PopupController::create_popover(const popover_descriptor& d) {
  static const float scale = zincbox::ui_scale();
  i32 space_needed = (8 + (4 + 24) * d.buttons.size() + 12) * scale;
  bool arrow_on_top = ui.get_window_height() - d.at.y >= space_needed;

  auto& popover = add_child<Popover>(arrow_on_top);
  popovers.emplace(d.id, &popover);

  popover.set_nine_slice_margin(8.0f);
  popover.set_anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM);
  popover.set_layout("ttb fit expand fill s:0");
  popover.get_layout().set_margin(4 * scale);

  popover.set_width(50 * scale);
  popover.arrow->set_is_drawn(d.show_arrow);
  if (arrow_on_top) {
    popover.set_pos(d.at.x, d.at.y + (d.distance + popover.arrow->get_height()));
  } else {
    popover.set_pos(d.at.x, d.at.y - (d.distance + popover.arrow->get_height()));
  }
  std::vector<Button*> buttons;
  std::string popover_id = d.id;

  if (!d.title.empty()) {
    auto& label = popover.add_child<Label>(d.title);
    label.update();
    popover.set_width(std::max<i32>(popover.get_width(), label.get_width() + 30 * scale));
    label.set_text_color(theme::config().text_color_muted);
    label.set_label_anchor(Anchor::TOP);
    label.set_resize_to_text_extents(false);
    label.set_min_height(20 * scale);
    label.set_max_height(20 * scale);
    label.set_height(20 * scale);
  }

  for (auto& [label, action, icon_id] : d.buttons) {
    auto& btn = popover.add_child<Button>(label);
    buttons.emplace_back(&btn);
    btn.set_nine_slice_margin(8.0f);
    btn.set_texture_idle("button_popover_idle");
    btn.set_texture_hovered("button_popover_hovered");
    btn.set_texture_pressed("button_popover_pressed");
    btn.set_texture_disabled("button_popover_disabled");
    btn.set_texture("button_popover_idle", false);
    btn.set_min_height(22 * scale);
    btn.set_max_height(22 * scale);
    btn.set_height(22 * scale);
    i32 w = btn.get_label().get_width() + 30;

    if (!icon_id.empty()) {
      auto& icon = btn.get_label().add_child<Sprite>(icon_id);
      btn.get_label().set_x(btn.get_label().get_x() + icon.get_width() / 2);
      icon.set_ignore_parents_layout(true);
      icon.set_ignore_parents_layout(true);
      icon.set_anchor(Anchor::RIGHT);
      icon.set_parent_anchor(Anchor::LEFT);
      icon.set_x(-4);
      w += icon.get_width();
    }

    popover.set_width(std::max(popover.get_width(), w));

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
  float dimmer_opacity = dimmer_visible ? 0.6f : 0.0f;
  dimmer->set_is_active(dimmer_block_events);
  dimmer->set_is_drawn(dimmer_visible || dimmer->get_opacity() > 0.01f);
  dimmer->set_opacity(std::lerp(dimmer->get_opacity(), dimmer_opacity, 0.25f));

  Widget::update();
}
