#pragma once
#include "core/settings.hpp"
#include "theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/ui.hpp"
#include "zincbox.hpp"

class ZincboxButton final : public zincgui::Button {
  public:
    ZincboxButton(zincgui::Root& ui_, const std::string& name_) : Button(ui_) {
      name = name_;
      init();
    }

    ZincboxButton(zincgui::Root& ui_, const std::string& name_, const ButtonConfig& config) : Button(ui_) {
      name = name_;
      init();
      set_nine_slice_margin(config.nine_slice_margin);
    }

    void set_texture_all(std::string texture_name) {
      set_texture_disabled(texture_name + "_disabled");
      set_texture_hovered(texture_name + "_hovered");
      set_texture_idle(texture_name + "_idle");
      set_texture_pressed(texture_name + "_pressed");
      set_texture(texture_name + "_idle", true);
    }

  private:
    void init() { set_texture_all(name); }
};

class ZincboxSlider final : public zincgui::Slider {
  public:
    ZincboxSlider(zincgui::Root& ui_, const std::string& name_) : Slider(ui_) {
      set_texture_thumb_pressed(name_ + "_thumb_pressed");
      set_texture_thumb_hovered(name_ + "_thumb_hovered");
      set_texture_thumb_idle(name_ + "_thumb_idle");
      set_texture_track_inactive(name_ + "_track_inactive");
      set_texture_track_active(name_ + "_track_active");
    }
};

class ZincboxScrollbar final : public zincgui::ScrollBar {
  public:
    ZincboxScrollbar(zincgui::Root& ui_) : ScrollBar(ui_) {
      static const float scale = zincbox::ui_scale();
      set_thumb_thickness(10 * scale);
      set_track_thickness(10 * scale);
      set_min_width(10 * scale);
      set_max_width(10 * scale);
      set_width(10 * scale);
    }

    void update() {
      sensitivity = zincbox::settings().interface.scrolling_speed;
      ScrollBar::update();
    }
};
