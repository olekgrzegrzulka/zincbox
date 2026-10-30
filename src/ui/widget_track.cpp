#include "widget_track.hpp"
#include <algorithm>
#include <string>
#include "common/logger.hpp"
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "core/settings.hpp"
#include "theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "zincbox.hpp"

using namespace zincgui;

WidgetTrack::WidgetTrack(zincgui::Root& ui_) : Button(ui_) {
  setup();
  update_highlight_status_from_player();

  m_slot_on_track_changed =
    player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
}

WidgetTrack::~WidgetTrack() { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

void WidgetTrack::update() {
  if (m_old_size != size()) {
    m_old_size = size();
    m_hover->size(size());
    m_bg->size(size());
  }

  Button::update();
}

void WidgetTrack::on_hovered() {
  m_hover->is_self_drawn(true);
  Button::on_hovered();
}

void WidgetTrack::on_unhovered() {
  m_hover->is_self_drawn(false);
  Button::on_unhovered();
}

void WidgetTrack::event(zincgui::Input::InputEventMouseMove& ev) { Button::event(ev); }

void WidgetTrack::update_highlight_status_from_player() {
  if (m_highlight_mode == TrackHighlightMode::QUEUE_INDEX) {
    highlighted(player::get_playing_index() == m_track_number - 1);
  } else if (m_highlight_mode == TrackHighlightMode::TRACK_INFO) {
    highlighted(player::get_playing_track() && m_track_id == player::get_playing_track()->track_id &&
                m_playlist_id == player::get_playing_track()->playlist_id &&
                m_collection_id == player::get_playing_track()->collection_id);
  }
}

void WidgetTrack::setup() {
  const float SCALE = zincbox::ui_scale();
  const float FONT_SIZE = zincbox::settings().interface.font_size;
  const i32 TRACK_HEIGHT = theme::config().panel_tracklist.track_height * SCALE;
  auto track = db::track_by_id(m_track_id);
  nine_slice_margin(8.0f)
    .fixed_height(0) // clear any fixed height set by the `Button`
    .height(TRACK_HEIGHT)
    .is_self_drawn(false)
    .layout("ltr mx:5 my:0 s:5 fill expand");

  label().is_drawn(false);

  if (!m_bg) {
    m_bg = &add_child<zincgui::ColorRect>(theme::config().panel_tracklist.track_color_odd);
    m_bg->ignore_parents_layout(true);
  }

  if (!m_hover) {
    m_hover = &add_child<zincgui::ColorRect>(theme::config().panel_tracklist.track_hovered);
    m_hover->opacity(theme::config().panel_tracklist.track_hovered_opacity)
      .ignore_parents_layout(true)
      .is_self_drawn(false);
  }

  if (!m_label_track_number) {
    m_label_track_number = &add_child<zincgui::Label>();
    m_label_track_number->label_anchor(zincgui::Anchor::LEFT)
      .text_color(theme::config().panel_tracklist.track_number_color);
  }
  m_label_track_number->text(std::to_string(m_track_number))
    .min_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_track_number->text_extents().x))
    .max_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_track_number->text_extents().x));

  if (!m_label_track_artist) {
    m_label_track_artist = &add_child<zincgui::Label>();
    m_label_track_artist->label_anchor(zincgui::Anchor::LEFT)
      .text_color(theme::config().panel_tracklist.track_artist_color);
  }
  if (track.has_value()) {
    if (track->get().metadata.artist.empty() || track->get().metadata.title.empty()) {
      m_label_track_artist->is_drawn(false)
        .text("")
        .min_width(0)
        .max_width(0);
    } else {
      m_label_track_artist->is_drawn(true)
        .text(track->get().metadata.artist)
        .min_width(0)
        .max_width(m_label_track_artist->text_extents().x);
    }
  }
  if (!m_label_track_title) {
    m_label_track_title = &add_child<zincgui::Label>();
    m_label_track_title->label_anchor(zincgui::Anchor::LEFT)
      .text_color(theme::config().panel_tracklist.title_color);
  }
  if (track.has_value()) { m_label_track_title->text(track->get().pretty_title()); }

  if (!m_love_icon) {
    m_love_icon = &add_child<Sprite>("love");
    m_love_icon->min_width(m_love_icon->width())
      .max_width(m_love_icon->width())
      .fixed_height(m_love_icon->height())
      .nine_slice_margin(0.0f);
  }
  m_love_icon->is_drawn(db::playlist_loved_tracks().has_track_id(m_track_id));

  if (!m_label_track_length) {
    m_label_track_length = &add_child<zincgui::Label>();
    m_label_track_length->label_anchor(zincgui::Anchor::RIGHT)
      .text_color(theme::config().panel_tracklist.length_color);
  }

  if (track.has_value()) { m_label_track_length->text(track->get().pretty_length()); }
  m_label_track_length->min_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_track_length->text_extents().x))
    .max_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_track_length->text_extents().x));

  if (track.has_value() && track->get().get_flag(db::TOMBSTONE)) {
    m_label_track_number->text_color(m_label_track_number->text_color() * 0.6f);
    m_label_track_artist->text_color(m_label_track_artist->text_color() * 0.6f);
    m_label_track_title->text_color(m_label_track_title->text_color() * 0.6f);
    m_label_track_length->text_color(m_label_track_length->text_color() * 0.6f);
  }

  playback_error(track.has_value() && track->get().get_flag(db::PLAYBACK_ERROR));
  update_text_colors();
}

void WidgetTrack::update_text_colors() {
  if (m_is_selected) {
    if (m_track_number % 2 == 0) {
      m_bg->color(theme::config().panel_tracklist.track_color_selected_odd);
    } else {
      m_bg->color(theme::config().panel_tracklist.track_color_selected_even);
    }
  } else if (m_highlighted) {
    m_bg->color(theme::config().panel_tracklist.track_color_playing);
  } else {
    if (m_track_number % 2 == 0) {
      m_bg->color(theme::config().panel_tracklist.track_color_odd);
    } else {
      m_bg->color(theme::config().panel_tracklist.track_color_even);
    }
  }

  m_label_track_number->text_color(theme::config().panel_tracklist.track_number_color);
  m_label_track_artist->text_color(theme::config().panel_tracklist.track_artist_color);
  m_label_track_title->text_color(theme::config().panel_tracklist.title_color);
  m_label_track_length->text_color(theme::config().panel_tracklist.length_color);

  auto track = db::track_by_id(m_track_id);
  if (track.has_value()) {
    if (m_is_selected) {
      m_label_track_number->text_color(m_label_track_number->text_color() * 5.0f);
      m_label_track_artist->text_color(m_label_track_artist->text_color() * 5.0f);
      m_label_track_title->text_color(m_label_track_title->text_color() * 5.0f);
      m_label_track_length->text_color(m_label_track_length->text_color() * 5.0f);
    } else if (track->get().get_flag(db::TOMBSTONE) || m_playback_error) {
      m_label_track_number->text_color(m_label_track_number->text_color() * 0.6f);
      m_label_track_artist->text_color(m_label_track_artist->text_color() * 0.6f);
      m_label_track_title->text_color(m_label_track_title->text_color() * 0.6f);
      m_label_track_length->text_color(m_label_track_length->text_color() * 0.6f);
    } else if (m_highlighted) {
      m_label_track_number->text_color(m_label_track_number->text_color() * 1.5f);
      m_label_track_artist->text_color(m_label_track_artist->text_color() * 1.5f);
      m_label_track_title->text_color(m_label_track_title->text_color() * 1.5f);
      m_label_track_length->text_color(m_label_track_length->text_color() * 1.5f);
    }
  }
}

db::track_info WidgetTrack::track_info() const {
  return db::track_info{.collection_id = m_collection_id,
                        .playlist_id = m_playlist_id,
                        .track_id = m_track_id,
                        .index = m_track_number - 1};
}

WidgetTrack& WidgetTrack::track_id(size_t value) {
  if (m_track_id != value) {
    m_track_id = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::playlist_id(size_t value) {
  if (m_playlist_id != value) {
    m_playlist_id = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::collection_id(size_t value) {
  if (m_collection_id != value) {
    m_collection_id = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::track_number(size_t value) {
  if (m_track_number != value) {
    m_track_number = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::is_selected(bool value) {
  if (m_is_selected != value) {
    m_is_selected = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::item_index(size_t value) {
  if (m_item_index != value) {
    m_item_index = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::highlight_mode(TrackHighlightMode value) {
  if (m_highlight_mode != value) {
    m_highlight_mode = value;
    setup();
    update_highlight_status_from_player();
  }
  return *this;
}

WidgetTrack& WidgetTrack::highlighted(bool highlighted_new) {
  if (m_highlighted == highlighted_new) { return *this; }
  m_highlighted = highlighted_new;
  update_text_colors();
  return *this;
}

WidgetTrack& WidgetTrack::playback_error(bool playback_error_new) {
  if (m_playback_error == playback_error_new) { return *this; }
  m_playback_error = playback_error_new;
  update_text_colors();
  return *this;
}
