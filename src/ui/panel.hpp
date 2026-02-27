#pragma once
#include "../input.hpp"
#include "../ui/sprite.hpp"

class Panel : public Sprite {
public:
  enum PanelStyle {
    Rectangular,
    RectangularDark,
    RectangularLight,
    Rounded,
    RoundedDark,
    RoundedLight,
    Invisible,
  };
  Panel(UI& ui_, PanelStyle style = PanelStyle::Rectangular, bool has_shadow = false) : Sprite::Sprite(ui_) {
    if (has_shadow) {
      shadow = &add_child<Sprite>("panel_shadow");
    }
    nine_slice_margin = 8;
    if (style == PanelStyle::Rectangular) {
      set_texture("panel_rectangular");
    }
    if (style == PanelStyle::Rounded) {
      set_texture("panel_rounded");
    }
    if (style == PanelStyle::RoundedDark) {
      set_texture("panel_rounded_dark");
    }
    if (style == PanelStyle::RoundedLight) {
      set_texture("panel_rounded_light");
    }
    if (style == PanelStyle::RectangularDark) {
      set_texture("panel_rectangular_dark");
    }
    if (style == PanelStyle::RectangularLight) {
      set_texture("panel_rectangular_light");
    }
    if (style == PanelStyle::Invisible) {
      set_is_self_drawn(false);
      if (has_shadow) {
        shadow->set_is_self_drawn(false);
      }
    }

    if (has_shadow) {
      shadow->set_ignore_parents_layout(true);
      shadow->set_draw_behind_parent(true);
      shadow->set_nine_slice_margin(8.0);
    }
  }

  void draw() override {
    Sprite::draw();
  }

  void update() override {
    if (shadow) {
      shadow->set_x(-4);
      shadow->set_y(-4);
      shadow->set_width(get_width() + 2 * 4);
      shadow->set_height(get_height() + 2 * 4);
    }
    Sprite::update();
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    if (is_mouse_hovering()) {
      ev.handled = true;
    }
  }

public:
  Sprite* shadow{};
};
