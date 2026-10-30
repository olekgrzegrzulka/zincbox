#pragma once
#include <functional>
#include <utility>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "ui/zincgui/color_rect.hpp"

class ZincboxButton;

namespace zincgui {
  class Button;
  class Label;
  class Slider;
  class Sprite;
  class ToolTip;
  class Root;
  class Widget;

  namespace Input {
    struct InputEventMouseButton;
    struct InputEventKey;
  } // namespace Input
} // namespace zincgui

class PanelControls final : public zincgui::ColorRect {
  public:
    PanelControls(const PanelControls&) = delete;
    PanelControls(PanelControls&&) = delete;
    PanelControls& operator=(const PanelControls&) = delete;
    PanelControls& operator=(PanelControls&&) = delete;

    PanelControls(zincgui::Root& ui_);
    ~PanelControls() override;

    using ColorRect::event;
    void event(zincgui::Input::InputEventMouseButton& ev) override;
    void event(zincgui::Input::InputEventKey& ev) override;
    void update() override;

    void update_love_state(bool is_loved);
    bool can_drag_window() const;

    PanelControls& button_expand_player_visibility(bool state);
    PanelControls& tooltip_visibility(bool state);
    PanelControls& disabled(bool dis);

    PanelControls& on_playing_track_lmb(std::function<void(Widget*)> fn) {
      m_on_playing_track_lmb = std::move(fn);
      return *this;
    }
    PanelControls& on_playing_track_rmb(std::function<void(Widget*)> fn) {
      m_on_playing_track_rmb = std::move(fn);
      return *this;
    }
    PanelControls& on_love_button_pressed(std::function<void()> fn) {
      m_on_love_button_pressed = std::move(fn);
      return *this;
    }
    PanelControls& on_button_expand_player_pressed(std::function<void()> fn);

  private:
    void update_repeat_mode();
    void update_shuffle_mode();
    void on_width_changed();
    void update_timestamp_label();
    void update_track_label();

    std::function<void(Widget*)> m_on_playing_track_lmb;
    std::function<void(Widget*)> m_on_playing_track_rmb;
    std::function<void()> m_on_love_button_pressed;

    zincgui::Button* m_button_play_pause{};
    zincgui::Button* m_button_stop{};
    zincgui::Button* m_button_next{};
    zincgui::Button* m_button_prev{};
    zincgui::Button* m_button_shuffle{};
    zincgui::Button* m_button_repeat{};
    zincgui::Button* m_button_expand_player{};
    zincgui::Slider* m_seekbar{};
    zincgui::Slider* m_volume_bar{};
    zincgui::Label* m_label_progress{};
    zincgui::Button* m_button_track{};
    zincgui::Label* m_label_track{};
    ZincboxButton* m_love_button{};

    bool m_tooltip_visibility = true;
    bool m_label_track_underline_lmb = false;
    bool m_label_track_underline_rmb = false;
    i32 m_progress_ms_prev = -1000;
    i32 m_total_ms_prev = -1000;
    bool m_compact_timestamp = false;
    bool m_hidden_timestamp = false;
    i32 m_tooltip_ms_prev = -1000;
    double m_volume_prev = -1.0;
    i32 m_width_prev = -1;
    i32 m_repeat_mode_prev = -1;
    i32 m_shuffle_mode_prev = -1;
    bool m_track_hovered = false;

    Signal<>::slot_key m_slot_on_track_changed;
    bool m_is_playing = false;
    bool m_disabled = false;
};
