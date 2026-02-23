#pragma once
#include "input.hpp"
#include "player.hpp"
#include "seekbar.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"

class PanelControls : public Panel {
public:
  PanelControls(UI& ui_) : Panel(ui_, PanelStyle::Rectangular, false) {
    set_anchor(Anchor::BOTTOM);
    set_parent_anchor(Anchor::BOTTOM);
    set_height(36);
    set_layout("m:4 s:4 ltr expand fill");

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

    button_prev = &add_child<Button>("");
    button_prev->set_max_width(36);
    auto& button_prev_img = button_prev->add_child<Sprite>("next");
    button_prev_img.set_anchor(Anchor::CENTER);
    button_prev_img.set_parent_anchor(Anchor::CENTER);

    // FIXME: don't use panel_bottom_right and just add the children to panel_bottom
    // we do this currently because the layout engine can't handle full-width element which is in the middle
    auto& panel_bottom_right = add_child<Widget>();
    panel_bottom_right.set_layout("m:0 s:4 rtl expand fill");

    auto& btn7 = panel_bottom_right.add_child<Button>("");
    btn7.set_max_width(36);
    auto& btn7_img = btn7.add_child<Sprite>("prev");
    btn7_img.set_anchor(Anchor::CENTER);
    btn7_img.set_parent_anchor(Anchor::CENTER);

    auto& btn8 = panel_bottom_right.add_child<Button>("");
    btn8.set_max_width(36);
    auto& btn8_img = btn8.add_child<Sprite>("prev");
    btn8_img.set_anchor(Anchor::CENTER);
    btn8_img.set_parent_anchor(Anchor::CENTER);

    auto& volume_bar = panel_bottom_right.add_child<Label>("VOL: 300%");
    volume_bar.set_max_width(70);

    auto& progress_label = panel_bottom_right.add_child<Label>("0:00 / 0:00");
    progress_label.set_max_width(80);

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
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    if (is_mouse_hovering()) {
      ev.handled = true;
    }
  }

  void update() override {
    Panel::update();
  }

public:
  Button* button_play{};
  Button* button_pause{};
  Button* button_stop{};
  Button* button_next{};
  Button* button_prev{};
  SeekBar* seekbar{};
};
