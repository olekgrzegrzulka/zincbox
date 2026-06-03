#include <string>
#include <string_view>
#include "common/input.hpp"
#include "common/types.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/widget.hpp"

Popup::Popup(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_) : Sprite(ui_, "panel_popup"), controller(controller_), on_close(on_close_) {
  set_is_drawn_on_top(true);
  set_nine_slice_margin(8.0f);
  set_parent_anchor(Anchor::CENTER);
  set_anchor(Anchor::CENTER);
}

void Popup::update() {
  if (is_dragged) {
    vec2i delta = Input::get_mouse_pos() - drag_start_mouse_pos;
    vec2i new_pos = drag_start_pos + delta;
    new_pos.x = std::clamp(new_pos.x, (width - ui.get_window_width()) / 2, (ui.get_window_width() - width) / 2);
    new_pos.y = std::clamp(new_pos.y, (height - ui.get_window_height()) / 2, (ui.get_window_height() - height) / 2);
    set_pos(new_pos);
  }
  Sprite::update();
}

void Popup::handle_event(Input::InputEventMouseButton& e) {
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

  if (!e.handled) {
    Sprite::handle_event(e);
  }
}
Popover::Popover(UI& ui_) : Sprite(ui_, "popover_panel") {
}

void Popover::update() {
  i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
  i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
  i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
  i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());
  set_x(get_x() + off_screen_left - off_screen_right);
  set_y(get_y() + off_screen_top - off_screen_bottom);
  
  Sprite::update();
}

void Popover::draw() {
  Sprite::draw();
}
