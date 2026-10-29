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
  color(theme::config().panel_controls.color);
  const float SCALE = zincbox::ui_scale();
  anchor(Anchor::BOTTOM)
    .parent_anchor(Anchor::BOTTOM)
    .height(theme::config().panel_controls.height * SCALE)
    .layout("ltr expand fill");
  layout().spacing = theme::config().panel_controls.padding * SCALE;
  layout().set_margin(theme::config().panel_controls.padding * SCALE);

  m_button_prev = &add_child<ZincboxButton>("prev", theme::config().panel_controls.button_previous);
  m_button_prev->max_width(theme::config().panel_controls.button_previous.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_previous.visible)
    .add_image("prev");

  m_button_play_pause = &add_child<ZincboxButton>("play_pause", theme::config().panel_controls.button_play);
  m_button_play_pause->max_width(theme::config().panel_controls.button_play.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_play.visible)
    .add_image("play");

  m_button_stop = &add_child<ZincboxButton>("stop", theme::config().panel_controls.button_stop);
  m_button_stop->max_width(theme::config().panel_controls.button_stop.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_stop.visible)
    .add_image("stop");

  m_button_next = &add_child<ZincboxButton>("next", theme::config().panel_controls.button_next);
  m_button_next->max_width(theme::config().panel_controls.button_next.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_next.visible)
    .add_image("next");

  auto& pad = add_child<Widget>();
  pad.min_width(5 * SCALE)
    .max_width(5 * SCALE);

  auto& panel_middle = add_child<Widget>();
  panel_middle.layout("m:0 s:4 btt expand fill")
    .clip_children(true);

  m_seekbar = &panel_middle.add_child<ZincboxSlider>("seekbar");
  i32 track_height = theme::config().panel_controls.seekbar.track_height;
  i32 thumb_width = theme::config().panel_controls.seekbar.thumb_width;
  i32 thumb_height = theme::config().panel_controls.seekbar.thumb_height;
  m_seekbar->is_scrollable(false)
    .is_drawn(theme::config().panel_controls.seekbar.visible)
    .track_nine_slice_margin(theme::config().panel_controls.seekbar.nine_slice_margin)
    .thumb_nine_slice_margin(0)
    .track_thickness(track_height)
    .drag_area_inflation(std::max(16 - track_height, 0))
    .thumb_thickness(thumb_height)
    .thumb_length(thumb_width)
    .thumb_constraint(ThumbConstraint::INSIDE_TRACK);
  m_tooltip_timestamp = &m_seekbar->add_child<ToolTip>("", ToolTipPosition::ABOVE, 4);

  auto& label_track_container = panel_middle.add_child<Widget>();
  label_track_container.layout("fill expand");

  m_button_track = &label_track_container.add_child<Button>("");
  m_button_track->label().is_drawn(false);
  m_button_track->layout("ltr s:4 fill expand fit")
    .is_self_drawn(false);
  m_button_track->on_press([this]() -> void {
    if (m_on_playing_track_lmb) { m_on_playing_track_lmb(m_button_track); }
  });
  m_button_track->on_press_rmb([this]() -> void {
    if (m_on_playing_track_rmb) { m_on_playing_track_rmb(m_button_track); }
  });

  m_love_button = &m_button_track->add_child<ZincboxButton>("panel_controls_love");
  m_love_button->anchor(Anchor::RIGHT)
    .parent_anchor(Anchor::LEFT)
    .size(12, 12)
    .fixed_width(12)
    .fixed_height(12)
    .nine_slice_margin(0.0f);
  m_love_button->on_press([this]() -> void {
    if (m_on_love_button_pressed) { m_on_love_button_pressed(); }
  });

  m_label_track = &m_button_track->add_child<Label>();
  m_label_track->label_anchor(Anchor::LEFT);

  auto& pad2 = add_child<Widget>();
  pad2.min_width(5 * SCALE)
    .max_width(5 * SCALE);

  m_label_progress = &add_child<Label>("0:00 / 0:00");

  m_volume_bar = &add_child<ZincboxSlider>("volume_bar");
  i32 vol_track_height = theme::config().panel_controls.volume_bar.track_height;
  i32 vol_thumb_width = theme::config().panel_controls.volume_bar.thumb_width;
  i32 vol_thumb_height = theme::config().panel_controls.volume_bar.thumb_height;
  m_volume_bar->is_drawn(theme::config().panel_controls.volume_bar.visible)
    .max_width(theme::config().panel_controls.volume_bar.width * SCALE)
    .track_nine_slice_margin(theme::config().panel_controls.volume_bar.nine_slice_margin)
    .thumb_nine_slice_margin(0)
    .track_thickness(vol_track_height)
    .drag_area_inflation(std::max(16 - vol_track_height, 0))
    .thumb_thickness(vol_thumb_height)
    .thumb_length(vol_thumb_width)
    .thumb_constraint(ThumbConstraint::INSIDE_TRACK)
    .min_value(0.0f)
    .max_value(1.0f);
  m_volume_bar->on_value_changed([](float, float volume) -> void { player::set_volume(volume); });
  m_volume_bar->on_drag_ended([](float, float volume) -> void { player::set_volume(volume); });
  m_tooltip_volume = &m_volume_bar->add_child<ToolTip>("", ToolTipPosition::ABOVE, -4);

  m_button_shuffle = &add_child<ZincboxButton>("shuffle", theme::config().panel_controls.button_shuffle);
  m_button_shuffle->max_width(theme::config().panel_controls.button_shuffle.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_shuffle.visible)
    .add_image("shuffle");
  update_shuffle_mode();
  m_tooltip_button_shuffle = &m_button_shuffle->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  m_button_repeat = &add_child<ZincboxButton>("repeat", theme::config().panel_controls.button_repeat);
  m_button_repeat->max_width(theme::config().panel_controls.button_repeat.width * SCALE)
    .fixed_height(0)
    .is_drawn(theme::config().panel_controls.button_repeat.visible)
    .add_image("repeat");
  update_repeat_mode();
  m_tooltip_button_repeat = &m_button_repeat->add_child<ToolTip>("", ToolTipPosition::ABOVE, 8);

  m_button_expand_player =
    &add_child<ZincboxButton>("expand_player", theme::config().panel_controls.button_expand_player);
  m_button_expand_player->max_width(theme::config().panel_controls.button_expand_player.width * SCALE)
    .add_image("expand_player");

  auto on_track_changed = [this]() -> void {
    auto playing = player::get_playing_track();
    if (!playing) {
      disabled(true);
      update_track_label();
    } else {
      disabled(false);
      update_track_label();
      bool is_loved = db::playlist_loved_tracks().has_track_id(playing->track_id);
      update_love_state(is_loved);
    }
    m_label_track->min_width(m_label_track->text_extents().x)
      .max_width(m_label_track->text_extents().x);
  };

  m_slot_on_track_changed = player::signal_on_track_changed.connect(on_track_changed);
  on_track_changed();

  m_seekbar->on_drag_ended([](float, float ms) {
    if (std::abs(player::get_display_time_ms() - static_cast<i32>(ms)) > 100) { player::seek_ms(ms); }
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

void PanelControls::update() {
  if (width() != m_width_prev) {
    m_width_prev = width();
    on_width_changed();
  }

  m_seekbar->min_value(0.0f)
    .max_value(static_cast<float>(player::get_total_duration_ms()));

  m_volume_bar->sensitivity(zincbox::settings().general.volume_step * 0.01f);
  if (!m_volume_bar->is_being_dragged()) { m_volume_bar->value(player::get_volume(), false); }

  update_timestamp_label();

  i32 progress_ms = m_seekbar->is_being_dragged() ? static_cast<i32>(m_seekbar->value()) : player::get_display_time_ms();
  if (!m_seekbar->is_being_dragged()) { m_seekbar->value(static_cast<float>(progress_ms), false); }
  if (m_seekbar->is_mouse_hovering()) {
    i32 x_rel = Input::get_mouse_x() - m_seekbar->position(Anchor::LEFT).x;
    i32 tooltip_ms = x_rel / (double)m_seekbar->width() * player::get_total_duration_ms();
    if (m_seekbar->thumb().is_mouse_hovering()) { tooltip_ms = static_cast<i32>(m_seekbar->value()); }
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
      m_tooltip_timestamp->text(ss_hover.str());
    }
    m_tooltip_timestamp->is_drawn(m_tooltip_visibility)
      .x(x_rel - (m_seekbar->width() / 2));
  } else {
    m_tooltip_timestamp->is_drawn(false);
  }

  if (m_volume_bar->is_mouse_hovering()) {
    double tooltip_value = m_volume_bar->value();
    if (m_volume_bar->thumb().is_mouse_hovering()) { tooltip_value = m_volume_bar->value(); }
    if (tooltip_value != m_volume_prev) {
      m_volume_prev = tooltip_value;
      m_tooltip_volume->text(std::to_string(static_cast<i32>(std::round(tooltip_value * 100))) + "%");
    }
    m_tooltip_volume->is_drawn(m_tooltip_visibility)
      .x(static_cast<i32>((m_volume_bar->value() - 0.5f) * m_volume_bar->width()));
  } else {
    m_tooltip_volume->is_drawn(false);
  }

  bool track_hovered = m_button_track->is_mouse_hovering();
  if (m_track_hovered != track_hovered) {
    m_track_hovered = track_hovered;
    update_track_label();
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
    m_tooltip_button_shuffle->is_drawn(m_tooltip_visibility);
    auto s = player::get_shuffle_mode();
    if (s == player::ShuffleMode::OFF) {
      m_tooltip_button_shuffle->text(tr::get("tooltip.shuffle.off"));
    } else if (s == player::ShuffleMode::ON) {
      m_tooltip_button_shuffle->text(tr::get("tooltip.shuffle.on"));
    }
  } else {
    m_tooltip_button_shuffle->is_drawn(false);
  }

  if (m_button_repeat->is_mouse_hovering()) {
    m_tooltip_button_repeat->is_drawn(m_tooltip_visibility);
    auto r = player::get_repeat_mode();
    if (r == player::RepeatMode::OFF) {
      m_tooltip_button_repeat->text(tr::get("tooltip.repeat.off"));
    } else if (r == player::RepeatMode::TRACK) {
      m_tooltip_button_repeat->text(tr::get("tooltip.repeat.track"));
    } else if (r == player::RepeatMode::ALBUM) {
      m_tooltip_button_repeat->text(tr::get("tooltip.repeat.album"));
    }
  } else {
    m_tooltip_button_repeat->is_drawn(false);
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

void PanelControls::update_love_state(bool is_loved) {
  if (is_loved) {
    m_love_button->texture_all("panel_controls_unlove");
  } else {
    m_love_button->texture_all("panel_controls_love");
  }
}

bool PanelControls::can_drag_window() const {
  return (is_mouse_hovering() && !m_button_play_pause->is_mouse_hovering() && !m_button_stop->is_mouse_hovering() &&
          !m_button_next->is_mouse_hovering() && !m_button_prev->is_mouse_hovering() &&
          !m_button_shuffle->is_mouse_hovering() && !m_button_repeat->is_mouse_hovering() &&
          !m_button_expand_player->is_mouse_hovering() && !m_seekbar->is_mouse_hovering() &&
          !m_volume_bar->thumb().is_mouse_hovering() && !m_volume_bar->track().is_mouse_hovering() &&
          !m_label_track->is_mouse_hovering() && !m_love_button->is_mouse_hovering());
}

PanelControls& PanelControls::button_expand_player_visibility(bool state) {
  m_button_expand_player->is_drawn(theme::config().panel_controls.button_expand_player.visible && state)
    .is_updated(m_button_expand_player->is_drawn());
  return *this;
}

PanelControls& PanelControls::tooltip_visibility(bool state) {
  m_tooltip_visibility = state;
  if (!state) {
    m_tooltip_button_shuffle->is_drawn(state);
    m_tooltip_button_repeat->is_drawn(state);
    m_tooltip_timestamp->is_drawn(state);
    m_tooltip_volume->is_drawn(state);
  }
  return *this;
}

PanelControls& PanelControls::disabled(bool dis) {
  m_disabled = dis;
  m_button_play_pause->disabled(dis);
  m_button_stop->disabled(dis);
  m_button_next->disabled(dis);
  m_button_prev->disabled(dis);
  m_seekbar->disabled(dis);
  update_track_label();
  m_love_button->is_drawn(!dis);
  return *this;
}

PanelControls& PanelControls::on_button_expand_player_pressed(std::function<void()> fn) {
  m_button_expand_player->on_press(std::move(fn));
  return *this;
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

void PanelControls::on_width_changed() {
  constexpr float BREAKPOINT_WIDTH = 650.0f;
  constexpr i32 MIN_BUTTON_WIDTH = 20;
  constexpr i32 MIN_VOLUME_BAR_WIDTH = 20;
  const float SCALE = zincbox::ui_scale();
  const auto& cfg = theme::config().panel_controls;

  float ratio = std::clamp(static_cast<float>(width()) / BREAKPOINT_WIDTH, 0.0f, 1.0f);

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
    button->max_width(calc_width(base_width, MIN_BUTTON_WIDTH));
  }

  update_timestamp_label();

  m_volume_bar->max_width(calc_width(theme::config().panel_controls.volume_bar.width, MIN_VOLUME_BAR_WIDTH));
}

void PanelControls::update_timestamp_label() {
  bool compact = width() <= 650;
  bool hidden = width() <= 500;
  i32 progress_ms = m_seekbar->is_being_dragged() ? static_cast<i32>(m_seekbar->value()) : player::get_display_time_ms();
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
    m_label_progress->text(ss.str())
      .fixed_width(m_label_progress->text_extents().x)
      .is_drawn(!hidden);
  }
}

void PanelControls::update_track_label() {
  if (m_disabled) {
    m_label_track->text("");
    return;
  }

  auto playing = player::get_playing_track();
  auto& track = db::track_by_id(playing->track_id)->get();

  if (m_track_hovered) {
    m_label_track->text("<u>" + track.pretty_name() + "</u>");
  } else {
    m_label_track->text(track.pretty_name());
  }
}
