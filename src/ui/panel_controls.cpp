#include "panel_controls.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <stdlib.h>
#include "common/color.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "core/settings.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

PanelControls::PanelControls(Root& ui_) : ColorRect(ui_) {

  set_color(theme::config().panel_controls.color);
  const float SCALE = zincbox::ui_scale();
  set_anchor(Anchor::BOTTOM);
  set_parent_anchor(Anchor::BOTTOM);
  set_height(theme::config().panel_controls.height * SCALE);
  set_layout("ltr expand fill");
  get_layout().spacing = theme::config().panel_controls.padding * SCALE;
  get_layout().set_margin(theme::config().panel_controls.padding * SCALE);

  m_button_prev = &add_child<ZincboxButton>("prev", theme::config().panel_controls.button_previous);
  m_button_prev->set_max_width(theme::config().panel_controls.button_previous.width * SCALE);
  m_button_prev->set_fixed_height(0);
  m_button_prev->set_is_drawn(theme::config().panel_controls.button_previous.visible);
  m_button_prev->add_image("prev");

  m_button_play_pause = &add_child<ZincboxButton>("play_pause", theme::config().panel_controls.button_play);
  m_button_play_pause->set_max_width(theme::config().panel_controls.button_play.width * SCALE);
  m_button_play_pause->set_fixed_height(0);
  m_button_play_pause->set_is_drawn(theme::config().panel_controls.button_play.visible);
  m_button_play_pause->add_image("play");

  m_button_stop = &add_child<ZincboxButton>("stop", theme::config().panel_controls.button_stop);
  m_button_stop->set_max_width(theme::config().panel_controls.button_stop.width * SCALE);
  m_button_stop->set_fixed_height(0);
  m_button_stop->set_is_drawn(theme::config().panel_controls.button_stop.visible);
  m_button_stop->add_image("stop");

  m_button_next = &add_child<ZincboxButton>("next", theme::config().panel_controls.button_next);
  m_button_next->set_max_width(theme::config().panel_controls.button_next.width * SCALE);
  m_button_next->set_fixed_height(0);
  m_button_next->set_is_drawn(theme::config().panel_controls.button_next.visible);
  m_button_next->add_image("next");

  auto& pad = add_child<Widget>();
  pad.set_min_width(5 * SCALE);
  pad.set_max_width(5 * SCALE);

  auto& panel_middle = add_child<Widget>();
  panel_middle.set_layout("m:0 s:4 btt expand fill");
  panel_middle.set_clip_children(true);

  m_seekbar = &panel_middle.add_child<ZincboxSlider>("seekbar");
  m_seekbar->set_is_scrollable(false);
  m_seekbar->set_is_drawn(theme::config().panel_controls.seekbar.visible);
  i32 track_height = theme::config().panel_controls.seekbar.track_height;
  i32 thumb_width = theme::config().panel_controls.seekbar.thumb_width;
  i32 thumb_height = theme::config().panel_controls.seekbar.thumb_height;
  m_seekbar->set_track_nine_slice_margin(theme::config().panel_controls.seekbar.nine_slice_margin);
  m_seekbar->set_thumb_nine_slice_margin(0);
  m_seekbar->set_track_thickness(track_height);
  m_seekbar->set_drag_area_inflation(std::max(16 - track_height, 0));
  m_seekbar->set_thumb_thickness(thumb_height);
  m_seekbar->set_thumb_length(thumb_width);
  m_seekbar->set_thumb_constraint(ThumbConstraint::INSIDE_TRACK);
  m_tooltip_timestamp = &m_seekbar->add_child<ToolTip>("", ToolTipPosition::ABOVE, 4);

  auto& label_track_container = panel_middle.add_child<Widget>();
  label_track_container.set_layout("ltr s:4 fill");

  m_love_button = &label_track_container.add_child<ZincboxButton>("panel_controls_love");
  m_love_button->set_anchor(Anchor::RIGHT);
  m_love_button->set_parent_anchor(Anchor::LEFT);
  m_love_button->set_size(12, 12);
  m_love_button->set_min_width(12);
  m_love_button->set_max_width(12);
  m_love_button->set_nine_slice_margin(0.0f);

  m_love_button->on_press([this]() -> void {
    if (on_love_button_pressed) { on_love_button_pressed(); }
  });

  m_label_track = &label_track_container.add_child<Label>();
  m_label_track->set_label_anchor(Anchor::LEFT);

  m_label_track_underline = &m_label_track->add_child<ColorRect>(theme::config().text_caret_color);
  m_label_track_underline->set_width(1);
  m_label_track_underline->set_height(1);
  m_label_track_underline->set_parent_anchor(Anchor::CENTER);
  m_label_track_underline->set_anchor(Anchor::CENTER);

  auto& pad2 = add_child<Widget>();
  pad2.set_min_width(5 * SCALE);
  pad2.set_max_width(5 * SCALE);

  m_label_progress = &add_child<Label>("0:00 / 0:00");

  m_volume_bar = &add_child<ZincboxSlider>("volume_bar");
  m_volume_bar->set_is_drawn(theme::config().panel_controls.volume_bar.visible);
  m_volume_bar->set_max_width(theme::config().panel_controls.volume_bar.width * SCALE);
  i32 vol_track_height = theme::config().panel_controls.volume_bar.track_height;
  i32 vol_thumb_width = theme::config().panel_controls.volume_bar.thumb_width;
  i32 vol_thumb_height = theme::config().panel_controls.volume_bar.thumb_height;
  m_volume_bar->set_track_nine_slice_margin(theme::config().panel_controls.volume_bar.nine_slice_margin);
  m_volume_bar->set_thumb_nine_slice_margin(0);
  m_volume_bar->set_track_thickness(vol_track_height);
  m_volume_bar->set_drag_area_inflation(std::max(16 - vol_track_height, 0));
  m_volume_bar->set_thumb_thickness(vol_thumb_height);
  m_volume_bar->set_thumb_length(vol_thumb_width);
  m_volume_bar->set_thumb_constraint(ThumbConstraint::INSIDE_TRACK);
  m_volume_bar->set_min_value(0.0f);
  m_volume_bar->set_max_value(1.0f);
  m_volume_bar->on_value_changed([](float, float volume) -> void { player::set_volume(volume); });
  m_volume_bar->on_drag_ended([](float, float volume) -> void { player::set_volume(volume); });
  m_tooltip_volume = &m_volume_bar->add_child<ToolTip>("", ToolTipPosition::ABOVE, -4);

  m_button_shuffle = &add_child<ZincboxButton>("shuffle", theme::config().panel_controls.button_shuffle);
  m_button_shuffle->set_max_width(theme::config().panel_controls.button_shuffle.width * SCALE);
  m_button_shuffle->set_fixed_height(0);
  m_button_shuffle->set_is_drawn(theme::config().panel_controls.button_shuffle.visible);
  m_button_shuffle->add_image("shuffle");
  update_shuffle_mode();
  m_tooltip_button_shuffle = &m_button_shuffle->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  m_button_repeat = &add_child<ZincboxButton>("repeat", theme::config().panel_controls.button_repeat);
  m_button_repeat->set_max_width(theme::config().panel_controls.button_repeat.width * SCALE);
  m_button_repeat->set_fixed_height(0);
  m_button_repeat->set_is_drawn(theme::config().panel_controls.button_repeat.visible);
  m_button_repeat->add_image("repeat");
  update_repeat_mode();
  m_tooltip_button_repeat = &m_button_repeat->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  m_button_expand_player =
    &add_child<ZincboxButton>("expand_player", theme::config().panel_controls.button_expand_player);
  m_button_expand_player->set_max_width(theme::config().panel_controls.button_expand_player.width * SCALE);
  m_button_expand_player->add_image("expand_player");

  auto on_track_changed = [this]() -> void {
    auto playing = player::get_playing_track();
    if (!playing.has_value()) {
      set_disabled(true);
    } else {
      set_disabled(false);
      auto& track = db::track_by_id(playing->track_id)->get();
      m_label_track->set_text(track.pretty_name());
      bool is_loved = db::playlist_loved_tracks().has_track_id(playing->track_id);
      update_love_state(is_loved);
    }
    m_label_track->set_min_width(m_label_track->get_text_extents().x);
    m_label_track->set_max_width(m_label_track->get_text_extents().x);
    m_label_track_underline->set_width(m_label_track->get_text_extents().x);
    m_label_track_underline->set_y(std::floor(m_label_track->get_text_extents().y / 2.0f));
  };

  m_slot_on_track_changed = player::signal_on_track_changed.connect(on_track_changed);
  on_track_changed();

  m_seekbar->on_drag_ended([](float, float ms) {
    if (std::abs(player::get_display_time_ms() - (i32)ms) > 100) { player::seek_ms(ms); }
  });

  m_button_play_pause->on_press([&]() {
    if (m_is_playing) {
      player::pause();
    } else {
      player::resume();
    }
  });

  m_button_stop->on_press([&]() { player::stop(); });

  m_button_prev->on_press([&]() { player::prev_track(); });

  m_button_next->on_press([&]() { player::next_track(); });

  m_button_repeat->on_press([this]() {
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

  m_button_shuffle->on_press([this]() {
    auto shuffle_mode = player::get_shuffle_mode();
    if (shuffle_mode == player::ShuffleMode::OFF) {
      player::set_shuffle_mode(player::ShuffleMode::ON);
    } else if (shuffle_mode == player::ShuffleMode::ON) {
      player::set_shuffle_mode(player::ShuffleMode::OFF);
    }

    update_shuffle_mode();
  });
}

PanelControls::~PanelControls() { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

void PanelControls::event(Input::InputEventMouseButton& ev) {
  if (is_mouse_hovering()) { ev.handled = true; }

  if (m_label_track_underline->get_is_drawn()) {
    if (ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
      if (ev.action == Input::MouseAction::PRESS) { m_label_track_underline_lmb = true; }
      if (ev.action == Input::MouseAction::RELEASE && m_label_track_underline_lmb) {
        m_label_track_underline_lmb = false;
        if (on_playing_track_lmb) { on_playing_track_lmb(m_label_track); }
      }
    } else if (ev.button == Input::MouseButton::MOUSE_BUTTON_RIGHT) {
      if (ev.action == Input::MouseAction::PRESS) { m_label_track_underline_rmb = true; }
      if (ev.action == Input::MouseAction::RELEASE && m_label_track_underline_rmb) {
        m_label_track_underline_rmb = false;
        if (on_playing_track_rmb) { on_playing_track_rmb(m_label_track); }
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
        player::seek_ms(player::get_display_time_ms() - 5000);
      }
      ev.handled = true;
    } else if (ev.key == Input::Key::KEY_RIGHT) {
      if (shift) {
        player::next_track();
      } else {
        player::seek_ms(player::get_display_time_ms() + 5000);
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

void PanelControls::on_width_changed() {
  constexpr float TIMESTAMP_COMPACT_WIDTH = 650.0f;
  constexpr float TIMESTAMP_HIDE_WIDTH = 500.0f;
  constexpr float BREAKPOINT_WIDTH = 650.0f;
  constexpr i32 MIN_BUTTON_WIDTH = 20;
  constexpr i32 MIN_VOLUME_BAR_WIDTH = 20;
  const float SCALE = zincbox::ui_scale();
  const auto& cfg = theme::config().panel_controls;

  float ratio = std::clamp(static_cast<float>(width) / BREAKPOINT_WIDTH, 0.0f, 1.0f);

  auto calc_width = [ratio, SCALE](i32 base_width, i32 min_width) -> i32 {
    const auto SCALED = static_cast<i32>(base_width * ratio);
    return static_cast<i32>(std::max(SCALED, min_width) * SCALE);
  };

  std::array buttons = {
    std::pair{m_button_prev, cfg.button_previous.width},
    std::pair{m_button_play_pause, cfg.button_play.width},
    std::pair{m_button_stop, cfg.button_stop.width},
    std::pair{m_button_next, cfg.button_next.width},
    std::pair{m_button_shuffle, cfg.button_shuffle.width},
    std::pair{m_button_repeat, cfg.button_repeat.width},
    std::pair{m_button_expand_player, cfg.button_expand_player.width},
  };

  for (const auto& [button, base_width] : buttons) {
    button->set_max_width(calc_width(base_width, MIN_BUTTON_WIDTH));
  }

  update_timestamp_label();

  m_volume_bar->set_max_width(calc_width(theme::config().panel_controls.volume_bar.width, MIN_VOLUME_BAR_WIDTH));
}

void PanelControls::update_timestamp_label() {
  bool compact = width <= 650;
  bool hidden = width <= 500;
  i32 progress_ms = m_seekbar->is_being_dragged() ? m_seekbar->get_value() : player::get_display_time_ms();
  i32 total_ms = player::get_total_duration_ms();
  i32 progress_s = progress_ms / 1000;
  i32 total_s = total_ms / 1000;

  if ((progress_s != m_progress_ms_prev / 1000) || (total_s != m_total_ms_prev / 1000) ||
      compact != m_compact_timestamp || hidden != m_hidden_timestamp) {
    m_progress_ms_prev = progress_s * 1000;
    m_total_ms_prev = total_s * 1000;
    m_compact_timestamp = compact;
    m_hidden_timestamp = hidden;

    i32 progress_m = progress_s / 60;
    progress_s %= 60;
    i32 total_m = total_s / 60;
    total_s %= 60;

    std::stringstream ss;
    ss << std::right << std::setfill('0');
    ss << std::setw(2) << progress_m << ":";
    ss << std::setw(2) << progress_s;
    if (!compact) {
      ss << " / ";
      ss << std::setw(2) << total_m << ":";
      ss << std::setw(2) << total_s;
    }
    m_label_progress->set_text(ss.str());
    m_label_progress->set_fixed_width(m_label_progress->get_text_extents().x);
    m_label_progress->set_is_drawn(!hidden);
  }
}

void PanelControls::update() {

  if (width != m_width_prev) {
    m_width_prev = width;
    on_width_changed();
  }

  m_seekbar->set_min_value(0);
  m_seekbar->set_max_value(player::get_total_duration_ms());

  m_volume_bar->set_sensitivity(zincbox::settings().general.volume_step * 0.01f);
  if (!m_volume_bar->is_being_dragged()) { m_volume_bar->set_value(player::get_volume(), false); }

  update_timestamp_label();

  i32 progress_ms = m_seekbar->is_being_dragged() ? m_seekbar->get_value() : player::get_display_time_ms();
  if (!m_seekbar->is_being_dragged()) { m_seekbar->set_value(progress_ms, false); }
  if (m_seekbar->is_mouse_hovering()) {
    i32 x_rel = Input::get_mouse_x() - m_seekbar->get_position(Anchor::LEFT).x;
    i32 tooltip_ms = x_rel / (double)m_seekbar->get_width() * player::get_total_duration_ms();
    if (m_seekbar->get_thumb().is_mouse_hovering()) { tooltip_ms = m_seekbar->get_value(); }
    if (m_seekbar->is_being_dragged()) { tooltip_ms = progress_ms; }
    i32 tooltip_s = tooltip_ms / 1000;
    if (tooltip_s != m_tooltip_ms_prev / 1000) {
      m_tooltip_ms_prev = tooltip_ms;
      i32 tooltip_m = tooltip_s / 60;
      tooltip_s %= 60;
      std::stringstream ss_hover;
      ss_hover << std::right << std::setfill('0');
      ss_hover << std::setw(2) << tooltip_m << ":";
      ss_hover << std::setw(2) << tooltip_s;
      m_tooltip_timestamp->set_text(ss_hover.str());
    }
    m_tooltip_timestamp->set_is_drawn(m_tooltip_visibility);
    // tooltip is centered - subtract half width to compensate
    m_tooltip_timestamp->set_x(x_rel - (m_seekbar->get_width() / 2));
  } else {
    m_tooltip_timestamp->set_is_drawn(false);
  }

  if (m_volume_bar->is_mouse_hovering()) {
    double tooltip_value = m_volume_bar->get_value();
    if (m_volume_bar->get_thumb().is_mouse_hovering()) { tooltip_value = m_volume_bar->get_value(); }
    if (tooltip_value != m_volume_prev) {
      m_volume_prev = tooltip_value;
      m_tooltip_volume->set_text(std::to_string((i32)(std::round(tooltip_value * 100))) + "%");
    }
    m_tooltip_volume->set_is_drawn(m_tooltip_visibility);
    // tooltip is centered - subtract half width to compensate
    m_tooltip_volume->set_x((m_volume_bar->get_value() - 0.5) * m_volume_bar->get_width());
  } else {
    m_tooltip_volume->set_is_drawn(false);
  }

  m_label_track_underline->set_is_drawn(m_label_track->is_mouse_hovering());
  if (!m_label_track_underline->get_is_drawn()) {
    m_label_track_underline_lmb = false;
    m_label_track_underline_rmb = false;
  }

  if (m_is_playing != player::is_playing()) {
    m_is_playing = player::is_playing();
    if (m_is_playing) {
      m_button_play_pause->add_image("pause");
    } else {
      m_button_play_pause->add_image("play");
    }
  }

  if (m_button_shuffle->is_mouse_hovering()) {
    m_tooltip_button_shuffle->set_is_drawn(m_tooltip_visibility);
    auto s = player::get_shuffle_mode();
    if (s == player::ShuffleMode::OFF) {
      m_tooltip_button_shuffle->set_text(tr::get("tooltip.shuffle.off"));
    } else if (s == player::ShuffleMode::ON) {
      m_tooltip_button_shuffle->set_text(tr::get("tooltip.shuffle.on"));
    }
  } else {
    m_tooltip_button_shuffle->set_is_drawn(false);
  }

  if (m_button_repeat->is_mouse_hovering()) {
    m_tooltip_button_repeat->set_is_drawn(m_tooltip_visibility);
    auto r = player::get_repeat_mode();
    if (r == player::RepeatMode::OFF) {
      m_tooltip_button_repeat->set_text(tr::get("tooltip.repeat.off"));
    } else if (r == player::RepeatMode::TRACK) {
      m_tooltip_button_repeat->set_text(tr::get("tooltip.repeat.track"));
    } else if (r == player::RepeatMode::ALBUM) {
      m_tooltip_button_repeat->set_text(tr::get("tooltip.repeat.album"));
    }
  } else {
    m_tooltip_button_repeat->set_is_drawn(false);
  }

  if (m_repeat_mode_prev != (i32)player::get_repeat_mode()) {
    update_repeat_mode();
    m_repeat_mode_prev = (i32)player::get_repeat_mode();
  }

  if (m_shuffle_mode_prev != (i32)player::get_shuffle_mode()) {
    update_shuffle_mode();
    m_shuffle_mode_prev = (i32)player::get_shuffle_mode();
  }

  ColorRect::update();
}

void PanelControls::update_repeat_mode() {
  auto repeat_mode = player::get_repeat_mode();
  if (repeat_mode == player::RepeatMode::OFF) {
    m_button_repeat->add_image("repeat_off");
  } else if (repeat_mode == player::RepeatMode::TRACK) {
    m_button_repeat->add_image("repeat_track");
  } else if (repeat_mode == player::RepeatMode::ALBUM) {
    m_button_repeat->add_image("repeat_album");
  }
}

void PanelControls::update_shuffle_mode() {
  auto shuffle_mode = player::get_shuffle_mode();
  if (shuffle_mode == player::ShuffleMode::OFF) {
    m_button_shuffle->add_image("shuffle_off");
  } else if (shuffle_mode == player::ShuffleMode::ON) {
    m_button_shuffle->add_image("shuffle");
  }
}

void PanelControls::update_love_state(bool is_loved) {
  if (is_loved) {
    m_love_button->set_texture_all("panel_controls_unlove");
  } else {
    m_love_button->set_texture_all("panel_controls_love");
  }
}

void PanelControls::set_button_expand_player_visibility(bool state) {
  m_button_expand_player->set_is_drawn(theme::config().panel_controls.button_expand_player.visible && state);
  m_button_expand_player->set_is_updated(m_button_expand_player->get_is_drawn());
}

void PanelControls::on_button_expand_player_pressed(std::function<void()> fn) {
  m_button_expand_player->on_press(std::move(fn));
}

void PanelControls::set_tooltip_visibility(bool state) {
  m_tooltip_visibility = state;
  if (!state) {
    m_tooltip_button_shuffle->set_is_drawn(state);
    m_tooltip_button_repeat->set_is_drawn(state);
    m_tooltip_timestamp->set_is_drawn(state);
    m_tooltip_volume->set_is_drawn(state);
  }
}

bool PanelControls::can_drag_window() const {
  return (is_mouse_hovering() && !m_button_play_pause->is_mouse_hovering() && !m_button_stop->is_mouse_hovering() &&
          !m_button_next->is_mouse_hovering() && !m_button_prev->is_mouse_hovering() &&
          !m_button_shuffle->is_mouse_hovering() && !m_button_repeat->is_mouse_hovering() &&
          !m_button_expand_player->is_mouse_hovering() && !m_seekbar->is_mouse_hovering() &&
          !m_volume_bar->get_thumb().is_mouse_hovering() && !m_volume_bar->get_track().is_mouse_hovering() &&
          !m_label_track->is_mouse_hovering() && !m_love_button->is_mouse_hovering());
}

void PanelControls::set_disabled(bool disabled) {
  m_button_play_pause->set_disabled(disabled);
  m_button_stop->set_disabled(disabled);
  m_button_next->set_disabled(disabled);
  m_button_prev->set_disabled(disabled);
  m_seekbar->set_disabled(disabled);
  if (disabled) { m_label_track->set_text(""); }
  m_love_button->set_is_drawn(!disabled);
}
