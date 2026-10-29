#include "zb_widgets.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"

using namespace zincgui;

ZincboxButton::ZincboxButton(zincgui::Root& ui_, const std::string& name_) : Button(ui_) {
  name(name_);
  init();
}

ZincboxButton::ZincboxButton(zincgui::Root& ui_, const std::string& name_, const ButtonConfig& config) : Button(ui_) {
  name(name_);
  init();
  nine_slice_margin(config.nine_slice_margin);
}

ZincboxButton& ZincboxButton::texture_all(const std::string& texture_name) {
  texture_disabled(texture_name + "_disabled")
    .texture_hovered(texture_name + "_hovered")
    .texture_idle(texture_name + "_idle")
    .texture_pressed(texture_name + "_pressed")
    .texture(texture_name + "_idle", true);
  return *this;
}

void ZincboxButton::init() { texture_all(name()); }

ZincboxSlider::ZincboxSlider(zincgui::Root& ui_, const std::string& name_) : Slider(ui_) {
  texture_thumb_pressed(name_ + "_thumb_pressed")
    .texture_thumb_hovered(name_ + "_thumb_hovered")
    .texture_thumb_idle(name_ + "_thumb_idle")
    .texture_track_inactive(name_ + "_track_inactive")
    .texture_track_active(name_ + "_track_active");
}
