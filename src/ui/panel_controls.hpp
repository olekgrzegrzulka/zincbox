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
    void event(Input::InputEventMouseScroll&) override;
    void event(Input::InputEventMouseButton&) override;
    void event(Input::InputEventKey&) override;
    void update() override;
    void update_love_state(bool);

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
    Sprite* button_repeat_img{};
    Sprite* button_shuffle_img{};
    ToolTip* tooltip_button_shuffle{};
    ToolTip* tooltip_button_repeat{};
    ToolTip* tooltip_timestamp{};
    ToolTip* tooltip_volume{};
    Slider* seekbar{};
    Slider* volume_bar{};
    Label* label_progress{};
    Label* label_track{};
    Sprite* love_icon{};
    Sprite* label_track_underline{};

    bool label_track_underline_lmb = false;
    bool label_track_underline_rmb = false;
    i32 progress_ms_prev = -1000;
    i32 total_ms_prev = -1000;
    i32 tooltip_ms_prev = -1000;
    double volume_prev = -1.0;

    i32 repeat_mode_prev = -1;
    i32 shuffle_mode_prev = -1;

    Signal<>::slot_key slot_on_track_changed;
    bool is_playing = false;

  protected:
    void update_repeat_mode();
    void update_shuffle_mode();
};
