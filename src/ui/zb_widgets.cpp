#include "zb_widgets.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"

using namespace zincgui;

ZincboxButton::ZincboxButton(zincgui::Root& ui_, const std::string& name_) : Button(ui_) {
  name = name_;
  init();
}

ZincboxButton::ZincboxButton(zincgui::Root& ui_, const std::string& name_, const ButtonConfig& config) : Button(ui_) {
  name = name_;
  init();
  set_nine_slice_margin(config.nine_slice_margin);
}

void ZincboxButton::set_texture_all(const std::string& texture_name) {
  set_texture_disabled(texture_name + "_disabled");
  set_texture_hovered(texture_name + "_hovered");
  set_texture_idle(texture_name + "_idle");
  set_texture_pressed(texture_name + "_pressed");
  set_texture(texture_name + "_idle", true);
}

void ZincboxButton::init() { set_texture_all(name); }

ZincboxSlider::ZincboxSlider(zincgui::Root& ui_, const std::string& name_) : Slider(ui_) {
  set_texture_thumb_pressed(name_ + "_thumb_pressed");
  set_texture_thumb_hovered(name_ + "_thumb_hovered");
  set_texture_thumb_idle(name_ + "_thumb_idle");
  set_texture_track_inactive(name_ + "_track_inactive");
  set_texture_track_active(name_ + "_track_active");
}
