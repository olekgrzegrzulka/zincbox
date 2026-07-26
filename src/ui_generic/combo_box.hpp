#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
#include <string>
#include <string_view>
#include "button.hpp"
#include "common/input.hpp"
#include "label.hpp"
#include "sprite.hpp"
#include "widget.hpp"

class ComboBoxItem : public Button {
  public:
    ComboBoxItem(UI& ui_) : Button(ui_) {
      set_parent_anchor(Anchor::TOP_CENTER);
      set_anchor(Anchor::TOP_CENTER);
      set_nine_slice_margin(2);
      set_texture_idle("combobox_item_idle");
      set_texture_hovered("combobox_item_hovered");
      set_texture_pressed("combobox_item_pressed");
      set_texture_disabled("combobox_item_disabled");
      set_nine_slice_margin(6.0);
      hover_test_parent = false;
      label.set_parent_anchor(Anchor::CENTER_LEFT);
      label.set_anchor(Anchor::CENTER_LEFT);
      label.set_label_anchor(Anchor::CENTER_LEFT);
      label.set_x(2);
    }
};

class ComboBox : public Button {
    using Button::event;

  public:
    ComboBox(UI& ui_)
      : Button(ui_), icon(add_child<Sprite>("combobox_expand")), dropdown_bg(add_child<Sprite>("panel_combobox")) {
      set_size(64, 24);
      set_nine_slice_margin(4.0);

      set_texture_idle("combobox_idle");
      set_texture_hovered("combobox_hovered");
      set_texture_pressed("combobox_pressed");
      set_texture_disabled("combobox_disabled");

      set_switch_mode(true);
      on_press([&] { on_button_pressed(); });
      on_depress([&] { on_button_depressed(); });

      icon.set_anchor(Anchor::RIGHT);
      icon.set_parent_anchor(Anchor::RIGHT);
      icon.set_nine_slice_margin(0.0f);
      icon.set_x(-8);

      label.set_x((-icon.get_width() - 8) / 2);

      dropdown_bg.set_parent_anchor(Anchor::BOTTOM_CENTER);
      dropdown_bg.set_anchor(Anchor::TOP_CENTER);
      dropdown_bg.set_clip_children(true);
      dropdown_bg.set_is_drawn_on_top(true);

      for (i32 i = 0; i < dropdown_max_length + 1; i += 1) {
        auto& item = dropdown_bg.add_child<ComboBoxItem>();
        item_widgets[i] = &item;
        item.set_height(item_height);
        item.on_press([&, i] { on_item_pressed(i); });
      }
    }

    void event(Input::InputEventMouseScroll& ev) override {
      if (dropdown_bg.is_mouse_hovering()) {
        target_scroll_progress += ev.offset.y * -1.0f;
        ev.handled = true;
      }
    }

    void update() override {
      Button::update();

      target_scroll_progress =
        std::clamp<float>(target_scroll_progress, 0.0f, std::max<i32>(0, items.size() - item_widgets.size() + 1));

      if (std::abs(scroll_progress - target_scroll_progress) > 0.02f) {
        scroll_progress = std::lerp(scroll_progress, target_scroll_progress, 0.4f);
      } else {
        scroll_progress = target_scroll_progress;
      }

      for (i32 i = 0; i < dropdown_max_length + 1; i += 1) {
        i32 widget_i = i;
        i32 label_i = i + (i32)scroll_progress;

        auto* item = item_widgets[widget_i];
        bool draw_item = label_i < (i32)items.size();
        if (draw_item) {
          item->set_width(width - 2 * dropdown_padding);
          item->set_height(item_height);
          float y_residue = (scroll_progress - std::trunc(scroll_progress)) * (item_height + dropdown_padding);
          item->set_y(dropdown_padding + (item_height + dropdown_padding) * widget_i - y_residue);

          item->get_label().set_text(items[label_i].second);
          item->set_is_updated(true);

        } else {
          item->set_is_updated(false);
        }
      }

      dropdown_animation_progress = std::min(dropdown_animation_progress + 0.3f, 1.0f);
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
        2 * dropdown_padding + (item_height + dropdown_padding) * std::min<i32>(items.size(), dropdown_max_length);
      float dropdown_animation_eased = std::sin(std::numbers::pi * (dropdown_animation_progress - 0.5f)) * 0.5 + 0.5;
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

    void draw() override { Button::draw(); }

    void event(Input::InputEventMouseButton& ev) override {
      if (!ev.handled) { Button::event(ev); }
    }

    void event(Input::InputEventKey& ev) override {
      if (!focused) { return; }
      if (ev.key == Input::Key::KEY_ESCAPE) {
        on_button_depressed();
      } else {
        ev.handled = true;
      }
    }

    void on_item_selected(std::function<void()> lambda_select_) { lambda_select = std::move(lambda_select_); }

    void add_item(std::string_view id, std::u32string_view label) { items.emplace_back(id, label); }

    std::string get_selected_item_id() const {
      if (selected_index < 0 || selected_index >= (i32)items.size()) { return ""; }
      return items[selected_index].first;
    }

    std::u32string get_selected_item_label() const {
      if (selected_index < 0 || selected_index >= (i32)items.size()) { return U""; }
      return items[selected_index].second;
    }

    i32 get_selected_index() const { return selected_index; }

    void select_item_by_label(std::u32string_view label) {
      for (i32 i = 0; i < (i32)items.size(); i += 1) {
        if (items[i].second == label) {
          on_item_pressed(i);
          break;
        }
      }
    }

    void select_item_by_id(std::string_view id) {
      for (i32 i = 0; i < (i32)items.size(); i += 1) {
        if (items[i].first == id) {
          on_item_pressed(i);
          break;
        }
      }
    }

    void select_item_by_index(i32 index) {
      if (index < 0 || index >= (i32)items.size()) { return; }
      on_item_pressed(index);
    }

  protected:
    void on_button_pressed() {
      dropdown_state = DropDownState::APPEARING;
      dropdown_animation_progress = 0.0f;
      icon.set_texture("combobox_contract");
    }

    void on_button_depressed() {
      dropdown_state = DropDownState::DISAPPEARING;
      dropdown_animation_progress = 0.0f;
      icon.set_texture("combobox_expand");
    }

    void on_item_pressed(i32 i) {
      if (i < 0 || i >= (i32)items.size()) { return; }
      i32 label_i = (i32)scroll_progress + i;
      selected_index = i;
      label.set_text(items[label_i].second);
      set_is_switched(false);

      if (lambda_select) { lambda_select(); }
    }

  protected:
    Sprite& icon;
    Sprite& dropdown_bg;
    i32 selected_index = -1;
    bool focused = false;

    std::function<void()> lambda_select = nullptr;

    i32 item_height = 20;
    i32 dropdown_padding = 1;
    static constexpr i32 dropdown_max_length = 10;

    enum class DropDownState : u8 { HIDDEN, APPEARING, DISAPPEARING, VISIBLE };
    DropDownState dropdown_state = ComboBox::DropDownState::HIDDEN;
    float dropdown_animation_progress = 0.0;
    float target_scroll_progress = 0.0;
    float scroll_progress = 0.0; // 1.0 per one item scrolled

    std::array<ComboBoxItem*, dropdown_max_length + 1>
      item_widgets; // +1 to accomodate for partially visible extra item
    std::vector<std::pair<std::string, std::u32string>> items;

  public:
    WIDGET_DEF_SETTER_DIRTY(item_height);
};
