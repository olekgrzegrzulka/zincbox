#pragma once
#include "ui_generic/button.hpp"
#include "ui_generic/slider.hpp"
#include "ui_generic/ui.hpp"

class ZincboxButton final : public Button {
  public:
    ZincboxButton(UI& ui_, std::string name) : Button(ui_) {
      set_texture_disabled(name + "_disabled");
      set_texture_hovered(name + "_hovered");
      set_texture_idle(name + "_idle");
      set_texture_pressed(name + "_pressed");
      set_texture(name + "_idle", true);
      set_nine_slice_margin(theme::get_button_nine_slice_margin(name));
    }
};

class ZincboxSlider final : public Slider {
  public:
    ZincboxSlider(UI& ui_, std::string name) : Slider(ui_) {
      set_texture_thumb_pressed(name + "_thumb_pressed");
      set_texture_thumb_hovered(name + "_thumb_hovered");
      set_texture_thumb_idle(name + "_thumb_idle");
      set_texture_track_inactive(name + "_track_inactive");
      set_texture_track_active(name + "_track_active");
    }
};
