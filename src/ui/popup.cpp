#include <algorithm>
#include <string>
#include <utility>
#include "common/types.hpp"
#include "ui/popup.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Popup::Popup(Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Sprite(ui_, "panel_popup"), controller(controller_), on_close(std::move(on_close_)) {
  set_nine_slice_margin(8.0f);
  set_parent_anchor(Anchor::CENTER);
  set_anchor(Anchor::CENTER);
}

void Popup::update() {
  if (is_dragged) {
    vec2i delta = Input::get_mouse_pos() - drag_start_mouse_pos;
    vec2i new_pos = drag_start_pos + delta;
    if (width < ui.get_window_width() && height < ui.get_window_height()) {
      new_pos.x = std::clamp(new_pos.x, (width - ui.get_window_width()) / 2, (ui.get_window_width() - width) / 2);
      new_pos.y = std::clamp(new_pos.y, (height - ui.get_window_height()) / 2, (ui.get_window_height() - height) / 2);
    }
    set_pos(new_pos);
    Input::set_cursor(Input::Cursor::HAND);
  } else {
    if (width < ui.get_window_width() && height < ui.get_window_height()) {
      set_x(std::clamp(get_x(), (width - ui.get_window_width()) / 2, (ui.get_window_width() - width) / 2));
      set_y(std::clamp(get_y(), (height - ui.get_window_height()) / 2, (ui.get_window_height() - height) / 2));
    }
    Input::reset_cursor();
  }
  Sprite::update();
}

void Popup::event(Input::InputEventMouseButton& e) {
  if (e.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
    if (e.action == Input::MouseAction::PRESS && is_mouse_hovering()) {
      is_dragged = true;
      e.handled = true;
      drag_start_pos = {x, y};
      drag_start_mouse_pos = Input::get_mouse_pos();
    } else if (e.action == Input::MouseAction::RELEASE) {
      is_dragged = false;
      e.handled = true;
    }
  }

  if (!e.handled) { Sprite::event(e); }
}

i32 Popup::get_max_content_width() const {
  constexpr float MIN_WINDOW_W = 480.0f;
  constexpr float NORMAL_WINDOW_W = 1200.0f;
  constexpr float MIN_MARGIN = 16.0f;
  constexpr float NORMAL_MARGIN = 160.0f;

  float window_w = static_cast<float>(ui.get_window_width());
  float t = std::clamp((window_w - MIN_WINDOW_W) / (NORMAL_WINDOW_W - MIN_WINDOW_W), 0.0f, 1.0f);
  i32 margin = static_cast<i32>(std::lerp(MIN_MARGIN, NORMAL_MARGIN, t));

  return std::max<i32>(200, ui.get_window_width() - margin);
}

i32 Popup::get_max_content_height() const {
  constexpr float MIN_WINDOW_H = 300.0f;
  constexpr float NORMAL_WINDOW_H = 800.0f;
  constexpr float MIN_MARGIN = 16.0f;
  constexpr float NORMAL_MARGIN = 120.0f;

  float window_h = static_cast<float>(ui.get_window_height());
  float t = std::clamp((window_h - MIN_WINDOW_H) / (NORMAL_WINDOW_H - MIN_WINDOW_H), 0.0f, 1.0f);
  i32 margin = static_cast<i32>(std::lerp(MIN_MARGIN, NORMAL_MARGIN, t));

  return std::max<i32>(100, ui.get_window_height() - margin);
}

Popover::Popover(Root& ui_, bool arrow_on_top) : Sprite(ui_, "popover_panel") {
  arrow = &add_child<Sprite>(arrow_on_top ? "popover_arrow" : "popover_arrow_inverted");
  arrow->set_ignore_parents_layout(true);
  arrow->set_parent_anchor(arrow_on_top ? Anchor::TOP : Anchor::BOTTOM);
  arrow->set_anchor(arrow_on_top ? Anchor::BOTTOM : Anchor::TOP);
  arrow->set_y(arrow_on_top ? 1 : -1);
}

void Popover::update() {
  i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
  i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
  i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
  i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());
  i32 push_x = off_screen_left - off_screen_right;
  i32 push_y = off_screen_top - off_screen_bottom;
  i32 lo = std::min(-width / 2 + 8, width / 2 - 8);
  i32 hi = std::max(-width / 2 + 8, width / 2 - 8);
  arrow_offset = std::clamp(arrow_offset - push_x, lo, hi);
  arrow->set_x(arrow_offset);
  set_x(get_x() + push_x);
  set_y(get_y() + push_y);

  Sprite::update();
}

void Popover::draw() { Sprite::draw(); }
