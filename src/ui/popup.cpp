#include "ui/popup.hpp"
#include <algorithm>
#include <string>
#include <utility>
#include "common/types.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Popup::Popup(Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_) :
    Sprite(ui_, "panel_popup"), m_controller(controller_), m_on_close(std::move(on_close_)) {
  hit_test_behavior(zincgui::HitTestBehavior::BLOCK)
    .nine_slice_margin(8.0f)
    .parent_anchor(Anchor::CENTER)
    .anchor(Anchor::CENTER);
  // is_focusable(true);
}

void Popup::update() {
  // if (!is_focused()) { m_is_dragged = false; }

  if (m_is_dragged) {
    vec2i delta = Input::get_mouse_pos() - m_drag_start_mouse_pos;
    vec2i new_pos = m_drag_start_pos + delta;
    if (width() < ui().get_window_width() && height() < ui().get_window_height()) {
      new_pos.x = std::clamp(new_pos.x, (width() - ui().get_window_width()) / 2, (ui().get_window_width() - width()) / 2);
      new_pos.y = std::clamp(new_pos.y, (height() - ui().get_window_height()) / 2, (ui().get_window_height() - height()) / 2);
    }
    pos(new_pos);
    Input::set_cursor(Input::Cursor::HAND);
  } else {
    if (width() < ui().get_window_width() && height() < ui().get_window_height()) {
      x(std::clamp(x(), (width() - ui().get_window_width()) / 2, (ui().get_window_width() - width()) / 2));
      y(std::clamp(y(), (height() - ui().get_window_height()) / 2, (ui().get_window_height() - height()) / 2));
    }
    Input::reset_cursor();
  }
  Sprite::update();
}

void Popup::event(Input::InputEventMouseButton& e) {
  if (e.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
    if (e.action == Input::MouseAction::PRESS && is_mouse_hovering()) {
      m_is_dragged = true;
      e.handled = true;
      m_drag_start_pos = {x(), y()};
      m_drag_start_mouse_pos = Input::get_mouse_pos();
    } else if (e.action == Input::MouseAction::RELEASE) {
      m_is_dragged = false;
      e.handled = true;
    }
  }

  if (!e.handled) { Sprite::event(e); }
}

void Popup::event(zincgui::Input::InputEventKey& e) {
  if (e.key == zincgui::Input::Key::KEY_ESCAPE && e.action == zincgui::Input::KeyAction::RELEASE) {
    close();
    e.handled = true;
  }
}

i32 Popup::max_content_width() const {
  constexpr float MIN_WINDOW_W = 480.0f;
  constexpr float NORMAL_WINDOW_W = 1200.0f;
  constexpr float MIN_MARGIN = 16.0f;
  constexpr float NORMAL_MARGIN = 160.0f;

  float window_w = static_cast<float>(ui().get_window_width());
  float t = std::clamp((window_w - MIN_WINDOW_W) / (NORMAL_WINDOW_W - MIN_WINDOW_W), 0.0f, 1.0f);
  i32 margin = static_cast<i32>(std::lerp(MIN_MARGIN, NORMAL_MARGIN, t));

  return std::max<i32>(200, ui().get_window_width() - margin);
}

i32 Popup::max_content_height() const {
  constexpr float MIN_WINDOW_H = 300.0f;
  constexpr float NORMAL_WINDOW_H = 800.0f;
  constexpr float MIN_MARGIN = 16.0f;
  constexpr float NORMAL_MARGIN = 120.0f;

  float window_h = static_cast<float>(ui().get_window_height());
  float t = std::clamp((window_h - MIN_WINDOW_H) / (NORMAL_WINDOW_H - MIN_WINDOW_H), 0.0f, 1.0f);
  i32 margin = static_cast<i32>(std::lerp(MIN_MARGIN, NORMAL_MARGIN, t));

  return std::max<i32>(100, ui().get_window_height() - margin);
}

Popover::Popover(Root& ui_, bool arrow_on_top) : Sprite(ui_, "popover_panel") {
  nine_slice_margin(14.0f);
  m_arrow = &add_child<Sprite>(arrow_on_top ? "popover_arrow" : "popover_arrow_inverted");
  m_arrow->ignore_parents_layout(true)
    .parent_anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM)
    .anchor(arrow_on_top ? Anchor::BOTTOM : Anchor::TOP)
    .y(arrow_on_top ? 2 : -2);
}

void Popover::update() {
  if (is_layout_dirty()) { return; }

  vec2i top_left = vec2i(x(), y()) - vec2i(vec2f{static_cast<float>(width()), static_cast<float>(height())} * anchor_to_uv(anchor()));

  i32 off_screen_left = std::max(0, 0 - top_left.x);
  i32 off_screen_right = std::max(0, (top_left.x + width()) - ui().get_window_width());
  i32 off_screen_top = std::max(0, 0 - top_left.y);
  i32 off_screen_bottom = std::max(0, (top_left.y + height()) - ui().get_window_height());
  i32 push_x = off_screen_left - off_screen_right;
  i32 push_y = off_screen_top - off_screen_bottom;
  i32 lo = std::min((-width() / 2) + 16, (width() / 2) - 16);
  i32 hi = std::max((-width() / 2) + 16, (width() / 2) - 16);
  m_arrow_offset = std::clamp(m_arrow_offset - push_x, lo, hi);
  m_arrow->x(m_arrow_offset);
  x(x() + push_x)
    .y(y() + push_y);

  Sprite::update();
}
