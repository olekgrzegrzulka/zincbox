#pragma once
#include <functional>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "ui_generic/color_rect.hpp"

class Button;
class Label;
class Slider;
class Sprite;
class ToolTip;
class UI;
class Widget;

class PanelControls : public ColorRect {
  public:
    PanelControls(UI& ui_);
    ~PanelControls() override;
    using ColorRect::event;
    void event(Input::InputEventMouseButton&) override;
    void event(Input::InputEventKey&) override;
    void update() override;
    void update_love_state(bool);
    void set_button_expand_player_visibility(bool);
    void set_tooltip_visibility(bool);
    bool can_drag_window() const;

  public:
    std::function<void(Widget*)> on_playing_track_lmb{};
    std::function<void(Widget*)> on_playing_track_rmb{};
    void on_button_expand_player_pressed(std::function<void()>);

  protected:
    Button* button_play_pause{};
    Button* button_stop{};
    Button* button_next{};
    Button* button_prev{};
    Button* button_shuffle{};
    Button* button_repeat{};
    Button* button_expand_player{};
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

    bool tooltip_visibility = true;
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
