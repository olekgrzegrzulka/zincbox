#include "panel_controls.hpp"
#include <iomanip>
#include <ios>
#include <sstream>
#include "common/input.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/player.hpp"
#include "ui/theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

PanelControls::PanelControls(UI& ui_) : Sprite(ui_, "panel_controls") {
  set_anchor(Anchor::BOTTOM);
  set_parent_anchor(Anchor::BOTTOM);
  set_height(theme::get_prop("controls_panel_height").as_i32());
  set_layout("ltr expand fill");
  get_layout().spacing = theme::get_prop("controls_panel_padding").as_i32();
  get_layout().margin = theme::get_prop("controls_panel_padding").as_i32();

  button_prev = &add_child<ZincboxButton>("prev");
  button_prev->set_max_width(theme::get_prop("prev_button_width").as_i32());
  auto& button_prev_img = button_prev->add_child<Sprite>("prev");
  button_prev_img.set_anchor(Anchor::CENTER);
  button_prev_img.set_parent_anchor(Anchor::CENTER);

  button_play_pause = &add_child<ZincboxButton>("play_pause");
  button_play_pause->set_max_width(theme::get_prop("play_pause_button_width").as_i32());
  button_play_pause_img = &button_play_pause->add_child<Sprite>("play");
  button_play_pause_img->set_anchor(Anchor::CENTER);
  button_play_pause_img->set_parent_anchor(Anchor::CENTER);

  button_stop = &add_child<ZincboxButton>("stop");
  button_stop->set_max_width(theme::get_prop("stop_button_width").as_i32());
  auto& button_stop_img = button_stop->add_child<Sprite>("stop");
  button_stop_img.set_anchor(Anchor::CENTER);
  button_stop_img.set_parent_anchor(Anchor::CENTER);

  button_next = &add_child<ZincboxButton>("next");
  button_next->set_max_width(theme::get_prop("next_button_width").as_i32());
  auto& button_next_img = button_next->add_child<Sprite>("next");
  button_next_img.set_anchor(Anchor::CENTER);
  button_next_img.set_parent_anchor(Anchor::CENTER);

  auto& pad = add_child<Widget>();
  pad.set_min_width(5);
  pad.set_max_width(5);

  auto& panel_right = add_child<Widget>();
  panel_right.set_layout("m:0 rtl expand fill");
  panel_right.get_layout().spacing = theme::get_prop("controls_panel_padding").as_i32();

  button_repeat = &panel_right.add_child<ZincboxButton>("repeat");
  button_repeat->set_max_width(theme::get_prop("repeat_button_width").as_i32());
  auto& button_repeat_img = button_repeat->add_child<Sprite>("repeat");
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
  button_repeat_tooltip = &button_repeat->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  button_shuffle = &panel_right.add_child<ZincboxButton>("shuffle");
  button_shuffle->set_max_width(theme::get_prop("shuffle_button_width").as_i32());
  auto& button_shuffle_img = button_shuffle->add_child<Sprite>("shuffle");
  auto s = player::get_shuffle_mode();
  if (s == player::ShuffleMode::OFF) {
    button_shuffle_img.set_texture("shuffle_off");
  } else if (s == player::ShuffleMode::ON) {
    button_shuffle_img.set_texture("shuffle");
  }
  button_shuffle_img.set_anchor(Anchor::CENTER);
  button_shuffle_img.set_parent_anchor(Anchor::CENTER);
  button_shuffle_tooltip = &button_shuffle->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  volume_bar = &panel_right.add_child<ZincboxSlider>("volume_bar");
  volume_bar->set_track_nine_slice_margin(theme::get_prop("volume_bar_track_nine_slice_margin").as_i32(6.0));
  volume_bar->set_thumb_nine_slice_margin(theme::get_prop("volume_bar_thumb_nine_slice_margin").as_i32(6.0));
  volume_bar->set_max_width(std::clamp(theme::get_prop("volume_bar_width").as_i32(70), 20, 200));
  i32 vol_track_height = std::clamp(theme::get_prop("volume_bar_track_height").as_i32(12), 1, 50);
  i32 vol_thumb_width = std::clamp(theme::get_prop("volume_bar_thumb_width").as_i32(12), 1, 50);
  i32 vol_thumb_height = std::clamp(theme::get_prop("volume_bar_thumb_height").as_i32(12), 1, 50);
  volume_bar->set_track_thickness(vol_track_height);
  volume_bar->set_drag_area_inflation(std::max(16 - vol_track_height, 0));
  volume_bar->set_thumb_thickness(vol_thumb_height);
  volume_bar->set_thumb_length(vol_thumb_width);
  volume_bar->set_thumb_constraint(ThumbConstraint::INSIDE_TRACK);
  volume_bar->set_min_value(0.0f);
  volume_bar->set_max_value(1.0f);
  volume_bar->on_value_changed([](float, float volume) {
    player::set_volume(volume);
  });

  label_progress = &panel_right.add_child<Label>("0:00 / 0:00");
  label_progress->set_max_width(80);

  auto& pad2 = panel_right.add_child<Widget>();
  pad2.set_min_width(5);
  pad2.set_max_width(5);

  auto& panel_middle = panel_right.add_child<Widget>();
  panel_middle.set_layout("m:0 s:4 btt expand fill");
  panel_middle.set_clip_children(true);

  seekbar = &panel_middle.add_child<ZincboxSlider>("seekbar");
  seekbar->set_track_nine_slice_margin(theme::get_prop("seekbar_track_nine_slice_margin").as_i32(6.0));
  seekbar->set_thumb_nine_slice_margin(theme::get_prop("seekbar_thumb_nine_slice_margin").as_i32(6.0));
  i32 track_height = std::clamp(theme::get_prop("seekbar_track_height").as_i32(12), 1, 50);
  i32 thumb_width = std::clamp(theme::get_prop("seekbar_thumb_width").as_i32(12), 1, 50);
  i32 thumb_height = std::clamp(theme::get_prop("seekbar_thumb_height").as_i32(12), 1, 50);
  seekbar->set_track_thickness(track_height);
  seekbar->set_drag_area_inflation(std::max(16 - track_height, 0));
  seekbar->set_thumb_thickness(thumb_height);
  seekbar->set_thumb_length(thumb_width);
  seekbar->set_thumb_constraint(ThumbConstraint::INSIDE_TRACK);

  label_track = &panel_middle.add_child<Label>();
  label_track->set_label_anchor(Anchor::LEFT);

  slot_on_track_changed = player::signal_on_track_changed.connect([this]() {
    auto playing = player::get_playing();
    if (!playing.has_value()) {
      label_track->set_text("");
    } else {
      auto& track = db::track_by_id(playing->track_id)->get();
      if (track.artist.empty() || track.title.empty()) {
        label_track->set_text(std::filesystem::path{track.path}.filename().string());
      } else {
        label_track->set_text(track.artist + U" - " + track.title);
      }
    }
  });

  seekbar->on_drag_ended([](float, float ms) {
    if (std::abs(player::get_current_time_ms() - (i32)ms) > 100) {
      player::seek_ms(ms);
    }
  });

  button_play_pause->on_press([&]() {
    if (is_playing) {
      player::pause();
    } else {
      player::resume();
    }
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

  button_repeat->on_press([&button_repeat_img]() {
    auto repeat_mode = player::get_repeat_mode();
    if (repeat_mode == player::RepeatMode::OFF) {
      player::set_repeat_mode(player::RepeatMode::TRACK);
      button_repeat_img.set_texture("repeat_track");
    } else if (repeat_mode == player::RepeatMode::TRACK) {
      player::set_repeat_mode(player::RepeatMode::ALBUM);
      button_repeat_img.set_texture("repeat_album");
    } else if (repeat_mode == player::RepeatMode::ALBUM) {
      player::set_repeat_mode(player::RepeatMode::OFF);
      button_repeat_img.set_texture("repeat_off");
    }
  });

  button_shuffle->on_press([&button_shuffle_img]() {
    auto shuffle_mode = player::get_shuffle_mode();
    if (shuffle_mode == player::ShuffleMode::OFF) {
      player::set_shuffle_mode(player::ShuffleMode::ON);
      button_shuffle_img.set_texture("shuffle");
    } else if (shuffle_mode == player::ShuffleMode::ON) {
      player::set_shuffle_mode(player::ShuffleMode::OFF);
      button_shuffle_img.set_texture("shuffle_off");
    }
  });
}

PanelControls::~PanelControls() {
  player::signal_on_track_changed.disconnect(slot_on_track_changed);
}

void PanelControls::handle_event(Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering()) {
    ev.handled = true;
  }
}

void PanelControls::handle_event(Input::InputEventKey& ev) {
  bool shift = Input::key_pressed(Input::Key::KEY_LEFT_SHIFT) || Input::key_pressed(Input::Key::KEY_RIGHT_SHIFT);
  if (ev.action == Input::KeyAction::PRESS) {
    if (ev.key == Input::Key::KEY_LEFT) {
      if (shift) {
        player::prev_track();
      } else {
        player::seek_ms(player::get_current_time_ms() - 5000);
      }
      ev.handled = true;
    } else if (ev.key == Input::Key::KEY_RIGHT) {
      if (shift) {
        player::next_track();
      } else {
        player::seek_ms(player::get_current_time_ms() + 5000);
      }
      ev.handled = true;
    } else if (ev.key == Input::Key::KEY_SPACE) {
      if (player::is_playing()) {
        player::pause();
      } else {
        player::resume();
      }
      ev.handled = true;
    } else if (ev.key == Input::Key::KEY_UP) {
      player::set_volume(player::get_volume() + 0.05f);
      ev.handled = true;
    } else if (ev.key == Input::Key::KEY_DOWN) {
      player::set_volume(player::get_volume() - 0.05f);
      ev.handled = true;
    }
  }
}

void PanelControls::update() {
  i32 current_time_s;

  seekbar->set_min_value(0);
  seekbar->set_max_value(player::get_total_duration_ms());

  if (!volume_bar->is_being_dragged()) {
    volume_bar->set_value(player::get_volume(), false);
  }

  if (seekbar->is_being_dragged()) {
    current_time_s = seekbar->get_value() / 1000;
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

  seekbar->set_value(player::get_current_time_ms(), false);

  label_progress->set_text(ss.str());
  label_progress->set_is_drawn(width > 650);

  if (is_playing != player::is_playing()) {
    is_playing = player::is_playing();
    if (is_playing) {
      button_play_pause_img->set_texture("pause");
    } else {
      button_play_pause_img->set_texture("play");
    }
  }

  if (button_shuffle->is_mouse_hovering()) {
    button_shuffle_tooltip->set_is_drawn(true);
    auto s = player::get_shuffle_mode();
    if (s == player::ShuffleMode::OFF) {
      button_shuffle_tooltip->set_text("Shuffle: Off");
    } else if (s == player::ShuffleMode::ON) {
      button_shuffle_tooltip->set_text("Shuffle: On");
    }
  } else {
    button_shuffle_tooltip->set_is_drawn(false);
  }

  if (button_repeat->is_mouse_hovering()) {
    button_repeat_tooltip->set_is_drawn(true);
    auto r = player::get_repeat_mode();
    if (r == player::RepeatMode::OFF) {
      button_repeat_tooltip->set_text("Repeat: Off");
    } else if (r == player::RepeatMode::TRACK) {
      button_repeat_tooltip->set_text("Repeat: Track");
    } else if (r == player::RepeatMode::ALBUM) {
      button_repeat_tooltip->set_text("Repeat: Album");
    }
  } else {
    button_repeat_tooltip->set_is_drawn(false);
  }

  Sprite::update();
}
