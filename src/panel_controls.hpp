#pragma once
#include "input.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/slider.hpp"
#include "ui/sprite.hpp"
#include "ui/tooltip.hpp"

class PanelControls : public Panel {
  public:
    PanelControls(UI& ui_);
    void handle_event(Input::InputEventMouseButton& ev) override;
    void update() override;

  protected:
    Button* button_play_pause{};
    Sprite* button_play_pause_img{};
    Button* button_stop{};
    Button* button_next{};
    Button* button_prev{};
    Button* button_shuffle{};
    Button* button_repeat{};
    ToolTip* button_shuffle_tooltip{};
    ToolTip* button_repeat_tooltip{};
    Slider* seekbar{};
    Slider* volume_bar{};
    Label* progress_label{};

    bool is_playing = false;
};
