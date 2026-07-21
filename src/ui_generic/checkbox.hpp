#pragma once
#include <functional>
#include <string_view>
#include <utility>
#include "src/ui_generic/ui.hpp"
#include "src/ui_generic/widget.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/texture_atlas.hpp"

class Checkbox : public Widget {
  public:
    Checkbox(UI& ui_) : Widget(ui_) { init(U""); }

    Checkbox(UI& ui_, std::u32string_view label_text) : Widget(ui_) { init(label_text); }

    void init(std::u32string_view label_text) {
      uv_checkbox_idle = ui.get_texture_atlas().get("checkbox_idle")->get();
      uv_checkbox_hovered = ui.get_texture_atlas().get("checkbox_hovered")->get();
      uv_checkbox_pressed = ui.get_texture_atlas().get("checkbox_pressed")->get();
      uv_checkbox_disabled = ui.get_texture_atlas().get("checkbox_disabled")->get();
      uv_checkbox_check = ui.get_texture_atlas().get("checkbox_check")->get();

      set_layout("ltr fill fit s:4");

      button = &add_child<Button>();
      button->set_ignore_parents_layout(true);
      button->set_is_drawn(false);
      button->set_switch_mode(true);
      button->on_press([this]() -> void { set_checked(true); });
      button->on_depress([this]() -> void { set_checked(false); });

      sprite_checkbox = &add_child<Sprite>("checkbox_idle");
      sprite_checkbox->set_min_width(sprite_checkbox->get_width());
      sprite_checkbox->set_max_width(sprite_checkbox->get_width());
      sprite_checkbox->set_min_height(sprite_checkbox->get_height());
      sprite_checkbox->set_max_height(sprite_checkbox->get_height());
      sprite_check = &sprite_checkbox->add_child<Sprite>("checkbox_check");
      sprite_check->set_parent_anchor(Anchor::CENTER);
      sprite_check->set_anchor(Anchor::CENTER);

      label = &add_child<Label>(label_text);
    }

    bool is_checked() const { return button->get_is_switched(); }

    void set_checked(bool value) {
      if (checked == value) { return; }
      checked = value;
      button->set_is_switched(checked);
      sprite_check->set_is_drawn(checked);
      if (m_on_value_changed) { m_on_value_changed(); }
    }

    void update() override {
      button->set_size(width, height);
      sprite_check->set_is_drawn(checked);

      switch (button->get_state()) {
      case ButtonState::IDLE:
        sprite_checkbox->set_uv_start(uv_checkbox_idle.start);
        sprite_checkbox->set_uv_end(uv_checkbox_idle.end);
        break;
      case ButtonState::HOVERED:
        sprite_checkbox->set_uv_start(uv_checkbox_hovered.start);
        sprite_checkbox->set_uv_end(uv_checkbox_hovered.end);
        break;
      case ButtonState::PRESSED:
        sprite_checkbox->set_uv_start(uv_checkbox_pressed.start);
        sprite_checkbox->set_uv_end(uv_checkbox_pressed.end);
        break;
      case ButtonState::DISABLED:
        sprite_checkbox->set_uv_start(uv_checkbox_disabled.start);
        sprite_checkbox->set_uv_end(uv_checkbox_disabled.end);
        break;
      }

      Widget::update();
    }

    void on_value_changed(std::function<void()> lambda) { m_on_value_changed = std::move(lambda); }

  protected:
    bool checked = false;
    std::function<void()> m_on_value_changed;
    Button* button{};
    Sprite* sprite_checkbox{};
    Sprite* sprite_check{};
    Label* label{};
    TextureAtlasData uv_checkbox_idle;
    TextureAtlasData uv_checkbox_hovered;
    TextureAtlasData uv_checkbox_pressed;
    TextureAtlasData uv_checkbox_disabled;
    TextureAtlasData uv_checkbox_check;
};
