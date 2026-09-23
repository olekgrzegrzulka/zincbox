#pragma once
#include <functional>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/color_rect.hpp"
namespace zincgui {
  class Button;
  class Label;
  class Slider;
  class Sprite;
  class ToolTip;
  class Root;
  class Widget;
} // namespace zincgui

class PanelControls : public zincgui::ColorRect {
  public:
    PanelControls(zincgui::Root& ui_);
    ~PanelControls() override;
    using ColorRect::event;
    void event(zincgui::Input::InputEventMouseButton&) override;
    void event(zincgui::Input::InputEventKey&) override;
    void update() override;
    void update_love_state(bool);
    void set_button_expand_player_visibility(bool);
    void set_tooltip_visibility(bool);
    bool can_drag_window() const;

  public:
    std::function<void(Widget*)> on_playing_track_lmb{};
    std::function<void(Widget*)> on_playing_track_rmb{};
    std::function<void()> on_love_button_pressed{};
    void on_button_expand_player_pressed(std::function<void()>);

  protected:
    zincgui::Button* button_play_pause{};
    zincgui::Button* button_stop{};
    zincgui::Button* button_next{};
    zincgui::Button* button_prev{};
    zincgui::Button* button_shuffle{};
    zincgui::Button* button_repeat{};
    zincgui::Button* button_expand_player{};
    zincgui::ToolTip* tooltip_button_shuffle{};
    zincgui::ToolTip* tooltip_button_repeat{};
    zincgui::ToolTip* tooltip_timestamp{};
    zincgui::ToolTip* tooltip_volume{};
    zincgui::Slider* seekbar{};
    zincgui::Slider* volume_bar{};
    zincgui::Label* label_progress{};
    zincgui::Label* label_track{};
    ZincboxButton* love_button{};
    zincgui::ColorRect* label_track_underline{};

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
