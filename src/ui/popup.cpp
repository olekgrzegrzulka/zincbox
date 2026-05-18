#include <string>
#include <string_view>
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
  set_layout("ttb expand fit m:12 s:12");
}

PopupOld::PopupOld(UI& ui_, PopupController& controller_) : Popup(ui_, controller_, nullptr), title(add_child<Label>()), content(add_child<Widget>()) {
  content.set_parent_anchor(Anchor::TOP);
  content.set_anchor(Anchor::TOP);
  content.set_layout("ttb expand fit m:0 s:4");
}

Popover::Popover(UI& ui_) : Sprite(ui_, "popover_panel") {
}

void Popover::update() {
  Sprite::update();
}

void Popover::draw() {
  i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
  i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
  i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
  i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());

  set_x(get_x() + off_screen_left - off_screen_right);
  set_y(get_y() + off_screen_top - off_screen_bottom);

  Sprite::draw();
}
