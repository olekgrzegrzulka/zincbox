#include "panel_controls.hpp"
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include "common/input.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/player.hpp"
#include "tr.hpp"
#include "ui/theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/tooltip.hpp"
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
  button_repeat_img = &button_repeat->add_child<Sprite>("repeat");
  update_repeat_mode();
  button_repeat_img->set_anchor(Anchor::CENTER);
  button_repeat_img->set_parent_anchor(Anchor::CENTER);
  tooltip_button_repeat = &button_repeat->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  button_shuffle = &panel_right.add_child<ZincboxButton>("shuffle");
  button_shuffle->set_max_width(theme::get_prop("shuffle_button_width").as_i32());
  button_shuffle_img = &button_shuffle->add_child<Sprite>("shuffle");
  update_shuffle_mode();
  button_shuffle_img->set_anchor(Anchor::CENTER);
  button_shuffle_img->set_parent_anchor(Anchor::CENTER);
  tooltip_button_shuffle = &button_shuffle->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

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
  volume_bar->on_value_changed([](float, float volume) { player::set_volume(volume); });

  tooltip_volume = &volume_bar->add_child<ToolTip>("", ToolTipPosition::ABOVE, -4);

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

  tooltip_timestamp = &seekbar->add_child<ToolTip>("", ToolTipPosition::ABOVE, 4);

  auto& label_track_container = panel_middle.add_child<Widget>();
  label_track_container.set_layout("ltr s:4");

  love_icon = &label_track_container.add_child<Sprite>("love");
  love_icon->set_anchor(Anchor::RIGHT);
  love_icon->set_parent_anchor(Anchor::LEFT);
  love_icon->set_size(12, 12);
  love_icon->set_nine_slice_margin(0.0f);
  love_icon->set_is_drawn(false);

  label_track = &label_track_container.add_child<Label>();
  label_track->set_resize_to_text_extents(true);
  label_track->set_label_anchor(Anchor::LEFT);

  label_track_underline = &label_track->add_child<Sprite>("text_input_caret");
  label_track_underline->set_ignore_parents_layout(true);
  label_track_underline->set_width(1);
  label_track_underline->set_height(1);

  auto on_track_changed = [this]() -> void {
    auto playing = player::get_playing();
    if (!playing.has_value()) {
      label_track->set_text("");
      update_love_state(false);
    } else {
      auto& track = db::track_by_id(playing->track_id)->get();
      if (track.artist.empty() || track.title.empty()) {
        label_track->set_text(std::filesystem::path{track.path}.filename().string());
      } else {
        label_track->set_text(track.artist + U" - " + track.title);
      }
      bool is_loved = db::playlist_loved_tracks().has_track_id(playing->track_id);
      update_love_state(is_loved);
    }
    label_track->update();
    label_track_underline->set_width(label_track->get_text_extents().x);
    label_track_underline->set_y(label_track->get_text_extents().y + 2);
  };

  slot_on_track_changed = player::signal_on_track_changed.connect(on_track_changed);
  on_track_changed();

  seekbar->on_drag_ended([](float, float ms) {
    if (std::abs(player::get_current_time_ms() - (i32)ms) > 100) { player::seek_ms(ms); }
  });

  button_play_pause->on_press([&]() {
    if (is_playing) {
      player::pause();
    } else {
      player::resume();
    }
  });

  button_stop->on_press([&]() { player::stop(); });

  button_prev->on_press([&]() { player::prev_track(); });

  button_next->on_press([&]() { player::next_track(); });

  button_repeat->on_press([this]() {
    auto repeat_mode = player::get_repeat_mode();
    if (repeat_mode == player::RepeatMode::OFF) {
      player::set_repeat_mode(player::RepeatMode::TRACK);
    } else if (repeat_mode == player::RepeatMode::TRACK) {
      player::set_repeat_mode(player::RepeatMode::ALBUM);
    } else if (repeat_mode == player::RepeatMode::ALBUM) {
      player::set_repeat_mode(player::RepeatMode::OFF);
    }

    update_repeat_mode();
  });

  button_shuffle->on_press([this]() {
    auto shuffle_mode = player::get_shuffle_mode();
    if (shuffle_mode == player::ShuffleMode::OFF) {
      player::set_shuffle_mode(player::ShuffleMode::ON);
    } else if (shuffle_mode == player::ShuffleMode::ON) {
      player::set_shuffle_mode(player::ShuffleMode::OFF);
    }

    update_shuffle_mode();
  });
}

PanelControls::~PanelControls() { player::signal_on_track_changed.disconnect(slot_on_track_changed); }

void PanelControls::event(Input::InputEventMouseScroll& ev) {
  if (volume_bar->is_mouse_hovering()) {
    i32 sign = ev.offset.y / std::abs(ev.offset.y);
    player::set_volume(player::get_volume() + sign * 0.03);
    ev.handled = true;
  }
}

void PanelControls::event(Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering()) { ev.handled = true; }

  if (label_track_underline->get_is_drawn()) {
    if (ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
      if (ev.action == Input::MouseAction::PRESS) { label_track_underline_lmb = true; }
      if (ev.action == Input::MouseAction::RELEASE && label_track_underline_lmb) {
        label_track_underline_lmb = false;
        if (on_playing_track_lmb) { on_playing_track_lmb(label_track); }
      }
    } else if (ev.button == Input::MouseButton::MOUSE_BUTTON_RIGHT) {
      if (ev.action == Input::MouseAction::PRESS) { label_track_underline_rmb = true; }
      if (ev.action == Input::MouseAction::RELEASE && label_track_underline_rmb) {
        label_track_underline_rmb = false;
        if (on_playing_track_rmb) { on_playing_track_rmb(label_track); }
      }
    }
  }
}

void PanelControls::event(Input::InputEventKey& ev) {
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

  seekbar->set_min_value(0);
  seekbar->set_max_value(player::get_total_duration_ms());

  if (!volume_bar->is_being_dragged()) { volume_bar->set_value(player::get_volume(), false); }

  i32 progress_ms = seekbar->is_being_dragged() ? seekbar->get_value() : player::get_current_time_ms();
  i32 total_ms = player::get_total_duration_ms();
  i32 progress_s = progress_ms / 1000;
  i32 total_s = total_ms / 1000;

  if ((progress_s != progress_ms_prev / 1000) || (total_s != total_ms_prev / 1000)) {
    i32 progress_m = progress_s / 60;
    progress_s %= 60;
    i32 total_m = total_s / 60;
    total_s %= 60;

    std::stringstream ss;
    ss << std::right << std::setfill('0');
    ss << std::setw(0) << progress_m << ":";
    ss << std::setw(2) << progress_s;
    ss << " / ";
    ss << std::setw(0) << total_m << ":";
    ss << std::setw(2) << total_s;
    label_progress->set_text(ss.str());
  }

  label_progress->set_is_drawn(width > 650);

  seekbar->set_value(player::get_current_time_ms(), false);

  if (seekbar->is_mouse_hovering()) {
    i32 x_rel = Input::get_mouse_x() - seekbar->get_position(Anchor::LEFT).x;
    i32 tooltip_ms = x_rel / (double)seekbar->get_width() * player::get_total_duration_ms();
    if (seekbar->get_thumb().is_mouse_hovering()) { tooltip_ms = seekbar->get_value(); }
    if (seekbar->is_being_dragged()) { tooltip_ms = progress_ms; }
    i32 tooltip_s = tooltip_ms / 1000;
    if (tooltip_s != tooltip_ms_prev / 1000) {
      tooltip_ms_prev = tooltip_ms;
      i32 tooltip_m = tooltip_s / 60;
      tooltip_s %= 60;
      std::stringstream ss_hover;
      ss_hover << std::right << std::setfill('0');
      ss_hover << std::setw(0) << tooltip_m << ":";
      ss_hover << std::setw(2) << tooltip_s;
      tooltip_timestamp->set_text(ss_hover.str());
    }
    tooltip_timestamp->set_is_drawn(true);
    // tooltip is centered - subtract half width to compensate
    tooltip_timestamp->set_x(x_rel - seekbar->get_width() / 2);
  } else {
    tooltip_timestamp->set_is_drawn(false);
  }

  if (volume_bar->is_mouse_hovering()) {
    double tooltip_value = volume_bar->get_value();
    if (volume_bar->get_thumb().is_mouse_hovering()) { tooltip_value = volume_bar->get_value(); }
    if (tooltip_value != volume_prev) {
      volume_prev = tooltip_value;
      tooltip_volume->set_text(std::to_string((i32)(tooltip_value * 100)) + "%");
    }
    tooltip_volume->set_is_drawn(true);
    // tooltip is centered - subtract half width to compensate
    tooltip_volume->set_x((volume_bar->get_value() - 0.5) * volume_bar->get_width());
  } else {
    tooltip_volume->set_is_drawn(false);
  }

  label_track_underline->set_is_drawn(label_track->is_mouse_hovering() &&
                                      Input::get_mouse_x() <
                                        label_track->get_position().x + label_track->get_text_extents().x);
  if (!label_track_underline->get_is_drawn()) {
    label_track_underline_lmb = false;
    label_track_underline_rmb = false;
  }

  if (is_playing != player::is_playing()) {
    is_playing = player::is_playing();
    if (is_playing) {
      button_play_pause_img->set_texture("pause");
    } else {
      button_play_pause_img->set_texture("play");
    }
  }

  if (button_shuffle->is_mouse_hovering()) {
    tooltip_button_shuffle->set_is_drawn(true);
    auto s = player::get_shuffle_mode();
    if (s == player::ShuffleMode::OFF) {
      tooltip_button_shuffle->set_text(tr::get("tooltip.shuffle.off"));
    } else if (s == player::ShuffleMode::ON) {
      tooltip_button_shuffle->set_text(tr::get("tooltip.shuffle.on"));
    }
  } else {
    tooltip_button_shuffle->set_is_drawn(false);
  }

  if (button_repeat->is_mouse_hovering()) {
    tooltip_button_repeat->set_is_drawn(true);
    auto r = player::get_repeat_mode();
    if (r == player::RepeatMode::OFF) {
      tooltip_button_repeat->set_text(tr::get("tooltip.repeat.off"));
    } else if (r == player::RepeatMode::TRACK) {
      tooltip_button_repeat->set_text(tr::get("tooltip.repeat.track"));
    } else if (r == player::RepeatMode::ALBUM) {
      tooltip_button_repeat->set_text(tr::get("tooltip.repeat.album"));
    }
  } else {
    tooltip_button_repeat->set_is_drawn(false);
  }

  if (repeat_mode_prev != (i32)player::get_repeat_mode()) {
    update_repeat_mode();
    repeat_mode_prev = (i32)player::get_repeat_mode();
  }

  if (shuffle_mode_prev != (i32)player::get_shuffle_mode()) {
    update_shuffle_mode();
    shuffle_mode_prev = (i32)player::get_shuffle_mode();
  }

  Sprite::update();
}

void PanelControls::update_repeat_mode() {
  auto repeat_mode = player::get_repeat_mode();
  if (repeat_mode == player::RepeatMode::OFF) {
    button_repeat_img->set_texture("repeat_off");
  } else if (repeat_mode == player::RepeatMode::TRACK) {
    button_repeat_img->set_texture("repeat_track");
  } else if (repeat_mode == player::RepeatMode::ALBUM) {
    button_repeat_img->set_texture("repeat_album");
  }
}

void PanelControls::update_shuffle_mode() {
  auto shuffle_mode = player::get_shuffle_mode();
  if (shuffle_mode == player::ShuffleMode::OFF) {
    button_shuffle_img->set_texture("shuffle_off");
  } else if (shuffle_mode == player::ShuffleMode::ON) {
    button_shuffle_img->set_texture("shuffle");
  }
}

void PanelControls::update_love_state(bool is_loved) { love_icon->set_is_drawn(is_loved); }
