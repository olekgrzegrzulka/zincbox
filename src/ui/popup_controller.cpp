#include "popup_controller.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "common/color.hpp"
#include "ui/popup.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

Dimmer::Dimmer(zincgui::Root& ui_) : ColorRect(ui_, 0x00000000) {
  hit_test_behavior(zincgui::HitTestBehavior::BLOCK);
}

void Dimmer::update() {
  if (!m_prev_window_size.has_value()) { m_prev_window_size = ui().get_window_size(); }
  if (m_prev_window_size != ui().get_window_size()) {
    m_prev_window_size = ui().get_window_size();
    if (m_on_pressed) { m_on_pressed(); }
  }
  size(ui().get_window_size());
  ColorRect::update();
}

void Dimmer::event(zincgui::Input::InputEventMouseButton& ev) {
  if (ev.action == zincgui::Input::MouseAction::PRESS) {
    ev.handled = true;
    if (m_on_pressed) { m_on_pressed(); }
  }
}

void Dimmer::event(zincgui::Input::InputEventMouseScroll& ev) {
  ev.handled = true;
  if (m_on_pressed) { m_on_pressed(); }
}

void Dimmer::event(zincgui::Input::InputEventMouseMove& ev) { ev.handled = true; }

void Dimmer::event(zincgui::Input::InputEventKey& ev) {
  if (ev.action == zincgui::Input::KeyAction::RELEASE && ev.key == zincgui::Input::Key::KEY_ENTER) {
    if (m_on_enter_pressed) { m_on_enter_pressed(); }
    ev.handled = true;
  } else if (ev.action == zincgui::Input::KeyAction::RELEASE && ev.key == zincgui::Input::Key::KEY_ESCAPE) {
    if (m_on_escape_pressed) { m_on_escape_pressed(); }
    ev.handled = true;
  }
}

Dimmer& Dimmer::is_active(bool active) {
  is_updated(active);
  if (!active) { m_prev_window_size = std::nullopt; }
  return *this;
}

PopupController::PopupController(Root& ui_) : Widget(ui_) {
  m_dimmer = &add_child<Dimmer>();
  m_dimmer->on_pressed([this]() { on_dimmer_pressed(); })
    .on_enter_pressed([this]() { on_dimmer_enter_pressed(); })
    .on_escape_pressed([this]() { on_dimmer_escape_pressed(); });
}

void PopupController::input() {
  if (children().size() >= 2) {
    children().back()->input();
    m_dimmer->input();
  }
}

void PopupController::update() {
  size(ui().get_window_size());
  bool dimmer_block_events = (m_popovers.size() + m_popups.size()) > 0;
  bool dimmer_visible = m_popups.size() > 0;
  float dimmer_opacity = dimmer_visible ? 0.6f : 0.0f;
  m_dimmer->is_active(dimmer_block_events)
    .is_self_drawn(dimmer_visible || m_dimmer->opacity() > 0.01f)
    .opacity(std::lerp(m_dimmer->opacity(), dimmer_opacity, 0.25f));

  Widget::update();
}

void PopupController::event(zincgui::Input::InputEventKey& e) {
  if (e.key == zincgui::Input::Key::KEY_ESCAPE && e.action == zincgui::Input::KeyAction::RELEASE) {
    close_all_popovers();
    e.handled = true;
  }
}

void PopupController::on_dimmer_pressed() { close_all_popovers(); }

void PopupController::on_dimmer_enter_pressed() {}

void PopupController::on_dimmer_escape_pressed() { close_all_popovers(); }

void PopupController::create_popover(const popover_descriptor& d) {
  float scale = zincbox::ui_scale();
  i32 space_needed = (8 + (4 + 24) * d.buttons.size() + 12) * scale;
  bool arrow_on_top = ui().get_window_height() - d.at.y >= space_needed;

  auto& popover = add_child<Popover>(arrow_on_top);
  if (auto it = m_popovers.find(d.id); it != m_popovers.end()) {
    it->second->is_marked_for_deletion(true);
    m_popovers.erase(it);
  }
  m_popovers.emplace(d.id, &popover);

  popover.nine_slice_margin(8.0f)
    .anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM)
    .layout("ttb fit expand fill s:0");
  popover.layout().set_margin(4 * scale);

  popover.width(50 * scale);
  popover.arrow()->is_drawn(d.show_arrow);
  if (arrow_on_top) {
    popover.pos(d.at.x, d.at.y + (d.distance + popover.arrow()->height()));
  } else {
    popover.pos(d.at.x, d.at.y - (d.distance + popover.arrow()->height()));
  }
  std::vector<Button*> buttons;
  std::string popover_id = d.id;

  if (!d.title.empty()) {
    auto& lbl = popover.add_child<Label>(d.title);
    popover.width(std::max<i32>(popover.width(), lbl.width() + (30 * scale)));
    lbl.text_color(theme::config().text_color_muted)
      .label_anchor(Anchor::TOP)
      .resize_to_text_extents(false)
      .fixed_height(20 * scale)
      .height(20 * scale);
  }

  for (auto& [btn_label, action, icon_id] : d.buttons) {
    auto& btn = popover.add_child<Button>(btn_label);
    buttons.emplace_back(&btn);
    btn.nine_slice_margin(8.0f)
      .texture_idle("button_popover_idle")
      .texture_hovered("button_popover_hovered")
      .texture_pressed("button_popover_pressed")
      .texture_disabled("button_popover_disabled")
      .texture("button_popover_idle", false)
      .fixed_height(22 * scale)
      .height(22 * scale);
    i32 w = btn.label().width() + 30;

    if (!icon_id.empty()) {
      auto& icon = btn.label().add_child<Sprite>(icon_id);
      btn.label().x(btn.label().x() + (icon.width() / 2));
      icon.ignore_parents_layout(true);
      icon.ignore_parents_layout(true);
      icon.anchor(Anchor::RIGHT)
        .parent_anchor(Anchor::LEFT)
        .x(-4);
      w += icon.width();
    }

    popover.width(std::max(popover.width(), w));

    auto lambda = [this, action, popover_id, &popover]() -> void {
      if (action) { action(); }
      m_popovers.erase(popover_id);
      popover.is_marked_for_deletion(true);
    };
    btn.on_press(lambda);
  }
}

bool PopupController::is_popup_open() const { return m_popups.size() > 0; }

void PopupController::close_all_popups() {
  for (auto [_, popover] : m_popovers) {
    popover->is_marked_for_deletion(true);
  }
  m_popovers.clear();

  for (auto popup : m_popups) {
    popup->is_marked_for_deletion(true);
  }
  m_popups.clear();
}

void PopupController::close_all_popovers() {
  for (auto [_, popover] : m_popovers) {
    popover->is_marked_for_deletion(true);
  }
  m_popovers.clear();
}

void PopupController::on_popup_closed(Popup* popup) {
  close_all_popovers();
  popup->is_marked_for_deletion(true);
  auto it = std::find(m_popups.begin(), m_popups.end(), popup);
  if (it != m_popups.end()) { m_popups.erase(it); }
}
