#pragma once
#include <iomanip>
#include <ios>
#include <sstream>
#include "debug.hpp"
#include "input.hpp"
#include "player.hpp"
#include "seekbar.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/slider.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"

class PanelControls : public Panel {
public:
  PanelControls(UI& ui_) : Panel(ui_, PanelStyle::Rectangular, false) {
    set_anchor(Anchor::BOTTOM);
    set_parent_anchor(Anchor::BOTTOM);
    set_height(46);
    set_layout("m:6 s:6 ltr expand fill");

    button_prev = &add_child<Button>("");
    button_prev->set_max_width(36);
    auto& button_prev_img = button_prev->add_child<Sprite>("prev");
    button_prev_img.set_anchor(Anchor::CENTER);
    button_prev_img.set_parent_anchor(Anchor::CENTER);

    button_play = &add_child<Button>("");
    button_play->set_max_width(36);
    auto& button_play_img = button_play->add_child<Sprite>("play");
    button_play_img.set_anchor(Anchor::CENTER);
    button_play_img.set_parent_anchor(Anchor::CENTER);

    button_pause = &add_child<Button>("");
    button_pause->set_max_width(36);
    auto& button_pause_img = button_pause->add_child<Sprite>("pause");
    button_pause_img.set_anchor(Anchor::CENTER);
    button_pause_img.set_parent_anchor(Anchor::CENTER);

    button_stop = &add_child<Button>("");
    button_stop->set_max_width(36);
    auto& button_stop_img = button_stop->add_child<Sprite>("stop");
    button_stop_img.set_anchor(Anchor::CENTER);
    button_stop_img.set_parent_anchor(Anchor::CENTER);

    button_next = &add_child<Button>("");
    button_next->set_max_width(36);
    auto& button_next_img = button_next->add_child<Sprite>("next");
    button_next_img.set_anchor(Anchor::CENTER);
    button_next_img.set_parent_anchor(Anchor::CENTER);

    // FIXME: don't use panel_bottom_right and just add the children to panel_bottom
    // we do this currently because the layout engine can't handle full-width element which is in the middle
    auto& panel_bottom_right = add_child<Widget>();
    panel_bottom_right.set_layout("m:0 s:6 rtl expand fill");

    auto& button_repeat = panel_bottom_right.add_child<Button>("");
    button_repeat.set_max_width(36);
    auto& button_repeat_img = button_repeat.add_child<Sprite>("repeat");
    auto r = player::get_repeat_mode();
    if (r == player::RepeatMode::OFF) {
      button_repeat_img.set_texture("repeat_off");
    } else if (r == player::RepeatMode::TRACK) {
      button_repeat_img.set_texture("repeat_track");
    } else if (r == player::RepeatMode::ALBUM) {
      button_repeat_img.set_texture("repeat_album");
    }
    button_repeat_img.set_anchor(Anchor::CENTER);
    button_repeat_img.set_parent_anchor(Anchor::CENTER);

    auto& btn8 = panel_bottom_right.add_child<Button>("");
    btn8.set_max_width(36);
    auto& btn8_img = btn8.add_child<Sprite>("shuffle");
    btn8_img.set_anchor(Anchor::CENTER);
    btn8_img.set_parent_anchor(Anchor::CENTER);

    auto& volume_bar = panel_bottom_right.add_child<Slider>();
    volume_bar.set_max_width(70);

    progress_label = &panel_bottom_right.add_child<Label>("0:00 / 0:00");
    progress_label->set_max_width(80);

    seekbar = &panel_bottom_right.add_child<SeekBar>();

    button_play->on_press([&]() {
      player::play();
    });

    button_pause->on_press([&]() {
      player::pause();
    });

    button_stop->on_press([&]() {
      player::stop();
    });

    button_prev->on_press([&]() {
      player::prev_track();
    });

    button_next->on_press([&]() {
      player::next_track();
    });

    button_repeat.on_press([&]() {
      auto r = player::get_repeat_mode();
      if (r == player::RepeatMode::OFF) {
        player::set_repeat_mode(player::RepeatMode::TRACK);
        button_repeat_img.set_texture("repeat_track");
      } else if (r == player::RepeatMode::TRACK) {
        player::set_repeat_mode(player::RepeatMode::ALBUM);
        button_repeat_img.set_texture("repeat_album");
      } else if (r == player::RepeatMode::ALBUM) {
        player::set_repeat_mode(player::RepeatMode::OFF);
        button_repeat_img.set_texture("repeat_off");
      }
    });
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    if (is_mouse_hovering()) {
      ev.handled = true;
    }
  }

  void update() override {
    i32 current_time_s;
    if (seekbar->get_is_thumb_dragged()) {
      current_time_s = seekbar->get_current_time_ms_from_thumb_pos() / 1000;
    } else {
      current_time_s = player::get_current_time_ms() / 1000;
    }
    i32 current_time_m = current_time_s / 60;
    current_time_s %= 60;

    i32 total_duration_s = player::get_total_duration_ms() / 1000;
    i32 total_duration_m = total_duration_s / 60;
    total_duration_s %= 60;

    std::stringstream ss;
    ss << std::right << std::setfill('0');
    ss << std::setw(0) << current_time_m << ":";
    ss << std::setw(2) << current_time_s;
    ss << " / ";
    ss << std::setw(0) << total_duration_m << ":";
    ss << std::setw(2) << total_duration_s;

    progress_label->set_text(ss.str());
    Panel::update();
  }

public:
  Button* button_play{};
  Button* button_pause{};
  Button* button_stop{};
  Button* button_next{};
  Button* button_prev{};
  SeekBar* seekbar{};
  Label* progress_label{};
};
