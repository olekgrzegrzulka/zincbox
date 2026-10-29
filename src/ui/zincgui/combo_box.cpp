#include "combo_box.hpp"
#include <algorithm>
#include <array>
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
  parent_anchor(Anchor::TOP_CENTER)
    .anchor(Anchor::TOP_CENTER)
    .texture_idle(default_settings::combobox_item_texture_idle)
    .texture_hovered(default_settings::combobox_item_texture_hovered)
    .texture_pressed(default_settings::combobox_item_texture_pressed)
    .texture_disabled(default_settings::combobox_item_texture_disabled)
    .nine_slice_margin(default_settings::combobox_item_nine_slice_margin)
    .hover_test_parent(false);
  label().parent_anchor(Anchor::CENTER_LEFT)
    .anchor(Anchor::CENTER_LEFT)
    .label_anchor(Anchor::CENTER_LEFT)
    .x(default_settings::combobox_item_label_offset_x);
}

ComboBox::ComboBox(Root& ui_) :
    Button(ui_),
    m_icon(add_child<Sprite>(default_settings::combobox_texture_expand)),
    m_dropdown_bg(add_child<Sprite>(default_settings::combobox_texture_panel)),
    m_sprite_focus(add_child<Sprite>(default_settings::combobox_texture_focus)) {
  size(default_settings::combobox_size);
  if (default_settings::combobox_fixed_height) { fixed_height(height()); }
  nine_slice_margin(default_settings::combobox_nine_slice_margin);
  m_sprite_focus.nine_slice_margin(default_settings::combobox_nine_slice_margin);
  is_focusable(true)
    .texture_idle(default_settings::combobox_texture_idle)
    .texture_hovered(default_settings::combobox_texture_hovered)
    .texture_pressed(default_settings::combobox_texture_pressed)
    .texture_disabled(default_settings::combobox_texture_disabled)
    .switch_mode(true);
  on_press([&] { on_button_pressed(); });
  on_depress([&] { on_button_depressed(); });

  m_icon.anchor(Anchor::RIGHT)
    .parent_anchor(Anchor::RIGHT)
    .nine_slice_margin(0.0f)
    .x(default_settings::combobox_icon_offset_x);
  label().anchor(Anchor::LEFT)
    .parent_anchor(Anchor::LEFT)
    .label_anchor(Anchor::LEFT);

  m_dropdown_bg.parent_anchor(Anchor::BOTTOM_CENTER)
    .anchor(Anchor::TOP_CENTER)
    .clip_children(true)
    .is_drawn_on_top(true)
    .nine_slice_margin(default_settings::combobox_nine_slice_margin);

  for (i32 i = 0; i < m_dropdown_max_length + 1; i += 1) {
    auto& item = m_dropdown_bg.add_child<ComboBoxItem>();
    m_item_widgets[i] = &item;
    item.height(m_item_height);
    item.on_press([&, i] { on_item_pressed(i); });
  }
}

void ComboBox::update() {
  Button::update();
  label().x(height() / 2);

  if (m_dropdown_state == DropDownState::VISIBLE && !is_focused()) { on_button_depressed(); }

  m_sprite_focus.size(width(), height())
    .is_drawn(is_focused());

  m_target_scroll_progress =
    std::clamp<float>(m_target_scroll_progress, 0.0f, std::max<i32>(0, m_items.size() - m_item_widgets.size() + 1));

  if (std::abs(m_scroll_progress - m_target_scroll_progress) > default_settings::combobox_scroll_snap_threshold) {
    m_scroll_progress = std::lerp(m_scroll_progress, m_target_scroll_progress, default_settings::combobox_scroll_lerp_weight);
  } else {
    m_scroll_progress = m_target_scroll_progress;
  }

  for (i32 i = 0; i < m_dropdown_max_length + 1; i += 1) {
    i32 widget_i = i;
    i32 label_i = i + (i32)m_scroll_progress;

    auto* item = m_item_widgets[widget_i];
    bool draw_item = label_i < (i32)m_items.size();
    if (draw_item) {
      item->width(width() - (2 * m_dropdown_padding))
        .height(m_item_height);
      float y_residue = (m_scroll_progress - std::trunc(m_scroll_progress)) * (m_item_height + m_dropdown_padding);
      item->y(m_dropdown_padding + ((m_item_height + m_dropdown_padding) * widget_i) - y_residue);

      item->label().text(m_items[label_i].second);
      item->is_updated(true);

    } else {
      item->is_updated(false);
    }
  }

  m_dropdown_animation_progress =
    std::min(m_dropdown_animation_progress + default_settings::combobox_dropdown_anim_speed, 1.0f);
  if (m_dropdown_animation_progress >= 1.0f) {
    if (m_dropdown_state == DropDownState::APPEARING) {
      m_dropdown_state = DropDownState::VISIBLE;
    } else if (m_dropdown_state == DropDownState::DISAPPEARING) {
      m_dropdown_state = DropDownState::HIDDEN;
      m_scroll_progress = 0.0f;
      m_target_scroll_progress = 0.0f;
    }
  }

  i32 dropdown_height =
    (2 * m_dropdown_padding) + ((m_item_height + m_dropdown_padding) * std::min<i32>(m_items.size(), m_dropdown_max_length));
  float dropdown_animation_eased = (std::sin(std::numbers::pi * (m_dropdown_animation_progress - 0.5f)) * 0.5f) + 0.5f;
  if (m_dropdown_state == DropDownState::APPEARING) {
    dropdown_height = std::lerp(0, dropdown_height, dropdown_animation_eased);
  } else if (m_dropdown_state == DropDownState::DISAPPEARING) {
    dropdown_height = std::lerp(dropdown_height, 0, dropdown_animation_eased);
  }

  m_dropdown_bg.is_drawn(m_dropdown_state != DropDownState::HIDDEN)
    .is_updated(m_dropdown_state != DropDownState::HIDDEN || m_dropdown_animation_progress < 1.0f)
    .width(width())
    .height(dropdown_height);
}

void ComboBox::event(Input::InputEventMouseScroll& ev) {
  if (!is_focused()) { return; }
  m_target_scroll_progress += ev.offset.y * -1.0f;
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

ComboBox& ComboBox::add_item(std::string_view id, std::string_view label_) {
  m_items.emplace_back(id, label_);
  return *this;
}

std::string ComboBox::selected_item_id() const {
  if (m_selected_index < 0 || m_selected_index >= (i32)m_items.size()) { return ""; }
  return m_items[m_selected_index].first;
}

std::string ComboBox::selected_item_label() const {
  if (m_selected_index < 0 || m_selected_index >= (i32)m_items.size()) { return ""; }
  return m_items[m_selected_index].second;
}

i32 ComboBox::selected_index() const { return m_selected_index; }

ComboBox& ComboBox::select_item_by_label(std::string_view label_) {
  for (i32 i = 0; i < (i32)m_items.size(); i += 1) {
    if (m_items[i].second == label_) {
      on_item_pressed(i);
      break;
    }
  }
  return *this;
}

ComboBox& ComboBox::select_item_by_id(std::string_view id) {
  for (i32 i = 0; i < (i32)m_items.size(); i += 1) {
    if (m_items[i].first == id) {
      on_item_pressed(i);
      break;
    }
  }
  return *this;
}

ComboBox& ComboBox::select_item_by_index(i32 index) {
  if (index < 0 || index >= (i32)m_items.size()) { return *this; }
  on_item_pressed(index);
  return *this;
}

void ComboBox::on_button_pressed() {
  m_dropdown_state = DropDownState::APPEARING;
  m_dropdown_animation_progress = 0.0f;
  m_icon.texture(default_settings::combobox_texture_contract);
}

void ComboBox::on_button_depressed() {
  m_dropdown_state = DropDownState::DISAPPEARING;
  m_dropdown_animation_progress = 0.0f;
  m_icon.texture(default_settings::combobox_texture_expand);
}

void ComboBox::on_item_pressed(i32 i) {
  if (i < 0 || i >= (i32)m_items.size()) { return; }
  i32 label_i = (i32)m_scroll_progress + i;
  m_selected_index = i;
  label().text(m_items[label_i].second);
  is_switched(false);

  if (m_on_item_selected) { m_on_item_selected(); }
}
