#pragma once
#include "common/input.hpp"
#include "common/signal.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/slider.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/tooltip.hpp"
#include "ui_generic/widget.hpp"

class PanelControls : public Sprite {
  public:
    PanelControls(UI& ui_);
    ~PanelControls() override;
    using Sprite::event;
    void event(Input::InputEventMouseButton&) override;
    void event(Input::InputEventKey&) override;
    void update() override;

  public:
    std::function<void(Widget*)> on_playing_track_lmb{};
    std::function<void(Widget*)> on_playing_track_rmb{};

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
    Label* label_progress{};
    Label* label_track{};
    Sprite* label_track_underline{};
    bool label_track_underline_lmb = false;
    bool label_track_underline_rmb = false;

    Signal<>::slot_key slot_on_track_changed;
    bool is_playing = false;
};
