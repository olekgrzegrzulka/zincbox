#pragma once
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"

struct ButtonConfig;

namespace zincgui {
  class Root;
} // namespace zincgui

class ZincboxButton final : public zincgui::Button {
  public:
    ZincboxButton(zincgui::Root& ui_, const std::string& name_);
    ZincboxButton(zincgui::Root& ui_, const std::string& name_, const ButtonConfig& config);
    void set_texture_all(const std::string& texture_name);

  private:
    void init();
};

class ZincboxSlider final : public zincgui::Slider {
  public:
    ZincboxSlider(zincgui::Root& ui_, const std::string& name_);
};

class ZincboxScrollbar final : public zincgui::ScrollBar {
  public:
    ZincboxScrollbar(zincgui::Root& ui_);
    void update() override;
};
