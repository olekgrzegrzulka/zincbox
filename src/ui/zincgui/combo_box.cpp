#include "combo_box.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
#include <string>
#include <string_view>
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

ComboBoxItem::ComboBoxItem(Root& ui_) : Button(ui_) {
  set_parent_anchor(Anchor::TOP_CENTER);
  set_anchor(Anchor::TOP_CENTER);
  set_texture_idle(default_settings::combobox_item_texture_idle);
  set_texture_hovered(default_settings::combobox_item_texture_hovered);
  set_texture_pressed(default_settings::combobox_item_texture_pressed);
  set_texture_disabled(default_settings::combobox_item_texture_disabled);
  set_nine_slice_margin(default_settings::combobox_item_nine_slice_margin);
  hover_test_parent = false;
  label.set_parent_anchor(Anchor::CENTER_LEFT);
  label.set_anchor(Anchor::CENTER_LEFT);
  label.set_label_anchor(Anchor::CENTER_LEFT);
  label.set_x(default_settings::combobox_item_label_offset_x);
}

ComboBox::ComboBox(Root& ui_)
  : Button(ui_), icon(add_child<Sprite>(default_settings::combobox_texture_expand)),
    dropdown_bg(add_child<Sprite>(default_settings::combobox_texture_panel)),
    sprite_focus(add_child<Sprite>(default_settings::combobox_texture_focus)) {
  set_size(default_settings::combobox_size);
  if (default_settings::combobox_fixed_height) { set_fixed_height(height); }
  set_nine_slice_margin(default_settings::combobox_nine_slice_margin);
  sprite_focus.set_nine_slice_margin(default_settings::combobox_nine_slice_margin);
  set_is_focusable(true);

  set_texture_idle(default_settings::combobox_texture_idle);
  set_texture_hovered(default_settings::combobox_texture_hovered);
  set_texture_pressed(default_settings::combobox_texture_pressed);
  set_texture_disabled(default_settings::combobox_texture_disabled);

  set_switch_mode(true);
  on_press([&] { on_button_pressed(); });
  on_depress([&] { on_button_depressed(); });

  icon.set_anchor(Anchor::RIGHT);
  icon.set_parent_anchor(Anchor::RIGHT);
  icon.set_nine_slice_margin(0.0f);
  icon.set_x(default_settings::combobox_icon_offset_x);
  label.set_anchor(Anchor::LEFT);
  label.set_parent_anchor(Anchor::LEFT);
  label.set_label_anchor(Anchor::LEFT);

  dropdown_bg.set_parent_anchor(Anchor::BOTTOM_CENTER);
  dropdown_bg.set_anchor(Anchor::TOP_CENTER);
  dropdown_bg.set_clip_children(true);
  dropdown_bg.set_is_drawn_on_top(true);
  dropdown_bg.set_nine_slice_margin(default_settings::combobox_nine_slice_margin);

  for (i32 i = 0; i < dropdown_max_length + 1; i += 1) {
    auto& item = dropdown_bg.add_child<ComboBoxItem>();
    item_widgets[i] = &item;
    item.set_height(item_height);
    item.on_press([&, i] { on_item_pressed(i); });
  }
}

void ComboBox::update() {
  Button::update();
  label.set_x(height / 2);

  if (dropdown_state == DropDownState::VISIBLE && !is_focused()) { on_button_depressed(); }

  sprite_focus.set_size(width, height);
  sprite_focus.set_is_drawn(is_focused());

  target_scroll_progress =
    std::clamp<float>(target_scroll_progress, 0.0f, std::max<i32>(0, items.size() - item_widgets.size() + 1));

  if (std::abs(scroll_progress - target_scroll_progress) > default_settings::combobox_scroll_snap_threshold) {
    scroll_progress = std::lerp(scroll_progress, target_scroll_progress, default_settings::combobox_scroll_lerp_weight);
  } else {
    scroll_progress = target_scroll_progress;
  }

  for (i32 i = 0; i < dropdown_max_length + 1; i += 1) {
    i32 widget_i = i;
    i32 label_i = i + (i32)scroll_progress;

    auto* item = item_widgets[widget_i];
    bool draw_item = label_i < (i32)items.size();
    if (draw_item) {
      item->set_width(width - (2 * dropdown_padding));
      item->set_height(item_height);
      float y_residue = (scroll_progress - std::trunc(scroll_progress)) * (item_height + dropdown_padding);
      item->set_y(dropdown_padding + ((item_height + dropdown_padding) * widget_i) - y_residue);

      item->get_label().set_text(items[label_i].second);
      item->set_is_updated(true);

    } else {
      item->set_is_updated(false);
    }
  }

  dropdown_animation_progress =
    std::min(dropdown_animation_progress + default_settings::combobox_dropdown_anim_speed, 1.0f);
  if (dropdown_animation_progress >= 1.0) {
    if (dropdown_state == DropDownState::APPEARING) {
      dropdown_state = DropDownState::VISIBLE;
    } else if (dropdown_state == DropDownState::DISAPPEARING) {
      dropdown_state = DropDownState::HIDDEN;
      scroll_progress = 0.0f;
      target_scroll_progress = 0.0f;
    }
  }

  i32 dropdown_height =
    (2 * dropdown_padding) + ((item_height + dropdown_padding) * std::min<i32>(items.size(), dropdown_max_length));
  float dropdown_animation_eased = (std::sin(std::numbers::pi * (dropdown_animation_progress - 0.5f)) * 0.5) + 0.5;
  if (dropdown_state == DropDownState::APPEARING) {
    dropdown_height = std::lerp(0, dropdown_height, dropdown_animation_eased);
  } else if (dropdown_state == DropDownState::DISAPPEARING) {
    dropdown_height = std::lerp(dropdown_height, 0, dropdown_animation_eased);
  }

  dropdown_bg.set_is_drawn(dropdown_state != DropDownState::HIDDEN);
  dropdown_bg.set_is_updated(dropdown_state != DropDownState::HIDDEN || dropdown_animation_progress < 1.0);

  dropdown_bg.set_width(width);
  dropdown_bg.set_height(dropdown_height);
  // debug_warn(items_to_draw);
}

void ComboBox::draw() { Button::draw(); }

void ComboBox::event(Input::InputEventMouseScroll& ev) {
  if (!is_focused()) { return; }
  target_scroll_progress += ev.offset.y * -1.0f;
  ev.handled = true;
}

void ComboBox::event(Input::InputEventMouseButton& ev) {
  if (!ev.handled) { Button::event(ev); }
}

void ComboBox::event(Input::InputEventKey& ev) {
  if (!is_focused()) { return; }
  if (ev.key == Input::Key::KEY_ESCAPE) {
    on_button_depressed();
  } else {
    ev.handled = true;
  }
}

void ComboBox::on_item_selected(std::function<void()> lambda_select_) { lambda_select = std::move(lambda_select_); }

void ComboBox::add_item(std::string_view id, std::string_view label_) { items.emplace_back(id, label_); }

std::string ComboBox::get_selected_item_id() const {
  if (selected_index < 0 || selected_index >= (i32)items.size()) { return ""; }
  return items[selected_index].first;
}

std::string ComboBox::get_selected_item_label() const {
  if (selected_index < 0 || selected_index >= (i32)items.size()) { return ""; }
  return items[selected_index].second;
}

i32 ComboBox::get_selected_index() const { return selected_index; }

void ComboBox::select_item_by_label(std::string_view label_) {
  for (i32 i = 0; i < (i32)items.size(); i += 1) {
    if (items[i].second == label_) {
      on_item_pressed(i);
      break;
    }
  }
}

void ComboBox::select_item_by_id(std::string_view id) {
  for (i32 i = 0; i < (i32)items.size(); i += 1) {
    if (items[i].first == id) {
      on_item_pressed(i);
      break;
    }
  }
}

void ComboBox::select_item_by_index(i32 index) {
  if (index < 0 || index >= (i32)items.size()) { return; }
  on_item_pressed(index);
}

void ComboBox::on_button_pressed() {
  dropdown_state = DropDownState::APPEARING;
  dropdown_animation_progress = 0.0f;
  icon.set_texture(default_settings::combobox_texture_contract);
}

void ComboBox::on_button_depressed() {
  dropdown_state = DropDownState::DISAPPEARING;
  dropdown_animation_progress = 0.0f;
  icon.set_texture(default_settings::combobox_texture_expand);
}

void ComboBox::on_item_pressed(i32 i) {
  if (i < 0 || i >= (i32)items.size()) { return; }
  i32 label_i = (i32)scroll_progress + i;
  selected_index = i;
  label.set_text(items[label_i].second);
  set_is_switched(false);

  if (lambda_select) { lambda_select(); }
}
