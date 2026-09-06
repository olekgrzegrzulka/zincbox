#pragma once
#include "core/settings.hpp"
#include "theme_config.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/slider.hpp"
#include "ui_generic/ui.hpp"

class ZincboxButton final : public Button {
  public:
    ZincboxButton(UI& ui_, const std::string& name) : Button(ui_) {
      this->name = name;
      init();
    }

    ZincboxButton(UI& ui_, const std::string& name, const ButtonConfig& config) : Button(ui_) {
      this->name = name;
      init();
      set_nine_slice_margin(config.nine_slice_margin);
    }

  private:
    void init() {
      set_texture_disabled(name + "_disabled");
      set_texture_hovered(name + "_hovered");
      set_texture_idle(name + "_idle");
      set_texture_pressed(name + "_pressed");
      set_texture(name + "_idle", true);
    }
};

class ZincboxSlider final : public Slider {
  public:
    ZincboxSlider(UI& ui_, const std::string& name) : Slider(ui_) {
      set_texture_thumb_pressed(name + "_thumb_pressed");
      set_texture_thumb_hovered(name + "_thumb_hovered");
      set_texture_thumb_idle(name + "_thumb_idle");
      set_texture_track_inactive(name + "_track_inactive");
      set_texture_track_active(name + "_track_active");
    }
};

class ZincboxScrollbar final : public ScrollBar {
  public:
    ZincboxScrollbar(UI& ui_) : ScrollBar(ui_) {
      static const float scale = settings::get().scale * 0.01f;
      set_thumb_thickness(10 * scale);
      set_track_thickness(10 * scale);
      set_min_width(10 * scale);
      set_max_width(10 * scale);
      set_width(10 * scale);
    }

    void update() {
      sensitivity = settings::get().scrolling_speed;
      ScrollBar::update();
    }
};
