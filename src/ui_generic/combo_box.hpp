#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
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
      set_texture_idle("button_combo_idle");
      set_texture_hovered("button_combo_hovered");
      set_texture_pressed("button_combo_pressed");
      set_texture_disabled("button_combo_disabled");
      set_nine_slice_margin(6.0);
      hover_test_parent = false;
      label.set_parent_anchor(Anchor::CENTER_LEFT);
      label.set_anchor(Anchor::CENTER_LEFT);
      label.set_label_anchor(Anchor::CENTER_LEFT);
      label.set_x(2);
    }
};

class ComboBox : public Sprite {
  public:
    ComboBox(UI& ui_)
      : Sprite(ui_), button(add_child<Button>()), button_icon(add_child<Sprite>()),
        dropdown_bg(add_child<Sprite>("panel_combo")), label_item(add_child<Label>()) {
      set_size(64, 24);
      set_texture("combo_box", false);
      set_nine_slice_margin(2);

      button.set_parent_anchor(Anchor::CENTER_RIGHT);
      button.set_anchor(Anchor::CENTER_RIGHT);
      button.set_is_drawn(false);
      button.set_switch_mode(true);
      button.on_press([&] { on_button_pressed(); });
      button.on_depress([&] { on_button_depressed(); });

      button_icon.set_parent_anchor(Anchor::CENTER_RIGHT);
      button_icon.set_anchor(Anchor::CENTER_RIGHT);
      button_icon.set_texture("combo_box_button_expand");
      button_icon.set_size(32, 32);

      dropdown_bg.set_parent_anchor(Anchor::BOTTOM_CENTER);
      dropdown_bg.set_anchor(Anchor::TOP_CENTER);
      dropdown_bg.set_clip_children(true);
      dropdown_bg.set_is_drawn_on_top(true);

      label_item.set_parent_anchor(Anchor::CENTER_LEFT);
      label_item.set_anchor(Anchor::CENTER_LEFT);
      label_item.set_label_anchor(Anchor::CENTER_LEFT);
      label_item.set_x(2);

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
      Sprite::update();

      target_scroll_progress =
        std::clamp<float>(target_scroll_progress, 0.0f, std::max<i32>(0, item_labels.size() - item_widgets.size() + 1));

      if (std::abs(scroll_progress - target_scroll_progress) > 0.02f) {
        scroll_progress = std::lerp(scroll_progress, target_scroll_progress, 0.4f);
      } else {
        scroll_progress = target_scroll_progress;
      }

      for (i32 i = 0; i < dropdown_max_length + 1; i += 1) {
        i32 widget_i = i;
        i32 label_i = i + (i32)scroll_progress;

        auto* item = item_widgets[widget_i];
        bool draw_item = label_i < (i32)item_labels.size();
        if (draw_item) {
          item->set_width(width - 2 * dropdown_padding);
          item->set_height(item_height);
          float y_residue = (scroll_progress - std::trunc(scroll_progress)) * (item_height + dropdown_padding);
          item->set_y(dropdown_padding + (item_height + dropdown_padding) * widget_i - y_residue);

          item->get_label().set_text(item_labels[label_i]);
          item->set_is_updated(true);

        } else {
          item->set_is_updated(false);
        }
      }

      button.set_size(width, height);

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

      i32 dropdown_height = 2 * dropdown_padding +
                            (item_height + dropdown_padding) * std::min<i32>(item_labels.size(), dropdown_max_length);
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

    void draw() override { Sprite::draw(); }

    void event(Input::InputEventMouseButton& ev) override {
      bool mouse_hovering = is_mouse_hovering();
      bool lmb_just_pressed =
        ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT && ev.action == Input::MouseAction::PRESS;
      bool rmb_just_pressed =
        ev.button == Input::MouseButton::MOUSE_BUTTON_RIGHT && ev.action == Input::MouseAction::PRESS;
      bool mmb_just_pressed =
        ev.button == Input::MouseButton::MOUSE_BUTTON_MIDDLE && ev.action == Input::MouseAction::PRESS;

      if (mouse_hovering && lmb_just_pressed && !focused) {
        on_button_pressed();
      } else if (!mouse_hovering && (lmb_just_pressed || rmb_just_pressed || mmb_just_pressed) && focused) {
        on_button_depressed();
      }
    }

    void event(Input::InputEventKey& ev) override {
      if (!focused) { return; }
      if (ev.key == Input::Key::KEY_ESCAPE) {
        on_button_depressed();
      } else {
        ev.handled = true;
      }
    }

    void on_item_selected(std::function<void(i32)> lambda_select_) { lambda_select = std::move(lambda_select_); }

    void add_item(std::u32string label) { item_labels.emplace_back(std::move(label)); }

    std::u32string get_selected_item() const { return label_item.get_text(); }

    i32 get_selected_index() const { return selected_index; }

    void select_item_by_label(std::u32string_view label) {
      for (i32 i = 0; i < (i32)item_labels.size(); ++i) {
        if (item_labels[i] == label) {
          on_item_pressed(i);
          break;
        }
      }
    }

    void select_item_by_index(i32 index) {
      if (index < 0 || index >= (i32)item_labels.size()) { return; }
      on_item_pressed(index);
    }

  protected:
    void on_button_pressed() {
      set_focused(true);
      dropdown_state = DropDownState::APPEARING;
      dropdown_animation_progress = 0.0f;
      button_icon.set_texture("combo_box_button_contract");
    }

    void on_button_depressed() {
      set_focused(false);
      dropdown_state = DropDownState::DISAPPEARING;
      dropdown_animation_progress = 0.0f;
      button_icon.set_texture("combo_box_button_expand");
    }

    void set_focused(bool value) {
      if (focused == value) { return; }
      focused = value;
      if (focused) {
        set_texture("combo_box_focused", false);
      } else {
        set_texture("combo_box", false);
      }
    }

    void on_item_pressed(i32 i) {
      if (Input::get_mouse_y() <= get_position(Anchor::BOTTOM_CENTER).y) { return; }
      i32 label_i = (i32)scroll_progress + i;
      selected_index = i;
      label_item.set_text(item_labels[label_i]);
      button.set_is_switched(false);

      if (lambda_select) { lambda_select(label_i); }
    }

  protected:
    Button& button;
    Sprite& button_icon;
    Sprite& dropdown_bg;
    Label& label_item;
    i32 selected_index = -1;
    bool focused = false;

    std::function<void(i32)> lambda_select = nullptr;

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
    std::vector<std::u32string> item_labels;

  public:
    WIDGET_DEF_SETTER_DIRTY(item_height);
};
