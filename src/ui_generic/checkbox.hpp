#pragma once
#include <string_view>
#include "src/ui_generic/ui.hpp"
#include "src/ui_generic/widget.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/texture_atlas.hpp"

class Checkbox : public Widget {
  public:
    Checkbox(UI& ui_) : Widget(ui_) {
      init(U"");
    }

    Checkbox(UI& ui_, std::u32string_view label_text) : Widget(ui_) {
      init(label_text);
    }

    void init(std::u32string_view label_text) {
      uv_checkbox_idle = ui.get_texture_atlas().get("checkbox_idle")->get();
      uv_checkbox_hovered = ui.get_texture_atlas().get("checkbox_hovered")->get();
      uv_checkbox_pressed = ui.get_texture_atlas().get("checkbox_pressed")->get();
      uv_checkbox_disabled = ui.get_texture_atlas().get("checkbox_disabled")->get();
      uv_checkbox_check = ui.get_texture_atlas().get("checkbox_check")->get();

      label = &add_child<Label>(label_text);
      label->set_anchor(Anchor::CENTER_LEFT);
      label->set_parent_anchor(Anchor::CENTER_LEFT);
      label->set_label_anchor(Anchor::CENTER_LEFT);

      button = &add_child<Button>();
      button->set_is_drawn(false);
      button->set_switch_mode(true);
      button->on_press([this]() {
        sprite_check->set_is_drawn(true);
      });
      button->on_depress([this]() {
        sprite_check->set_is_drawn(false);
      });
      sprite_checkbox = &add_child<Sprite>("checkbox_idle");
      sprite_checkbox->set_anchor(Anchor::CENTER_LEFT);
      sprite_checkbox->set_parent_anchor(Anchor::CENTER_LEFT);
      sprite_check = &sprite_checkbox->add_child<Sprite>("checkbox_check");
      sprite_check->set_parent_anchor(Anchor::CENTER);
      sprite_check->set_anchor(Anchor::CENTER);
    }

    bool is_checked() const {
      return sprite_check->get_is_drawn();
    }

    void update() override {
      i32 inside_width = sprite_check->get_width() + 6 + label->get_width();
      i32 x_ = (width - inside_width) / 2;
      sprite_checkbox->set_pos(x_, 0);
      label->set_pos(x_ + sprite_check->get_width() + 6, 0);

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

      button->set_size(width, height);
      Widget::update();
    }

  protected:
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
