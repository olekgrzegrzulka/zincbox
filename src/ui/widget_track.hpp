#pragma once
#include <string>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "core/settings.hpp"
#include "core/zincbox.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/color_rect.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"

class WidgetTrack final : public Button {
  public:
    enum class TrackHighlightMode : u8 { TRACK_INFO, QUEUE_INDEX, OFF };
    WidgetTrack(UI& ui_) : Button(ui_) {
      setup();

      slot_on_track_changed =
        player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
    }

    void update_highlight_status_from_player() {
      if (m_highlight_mode == TrackHighlightMode::QUEUE_INDEX) {
        set_highlighted(player::get_playing_index().has_value() &&
                        (m_track_number - 1) == player::get_playing_index().value());
      } else if (m_highlight_mode == TrackHighlightMode::TRACK_INFO) {
        set_highlighted(player::get_playing().has_value() && m_track_id == player::get_playing()->track_id &&
                        m_playlist_id == player::get_playing()->playlist_id &&
                        m_collection_id == player::get_playing()->collection_id);
      }
    }

    ~WidgetTrack() override { player::signal_on_track_changed.disconnect(slot_on_track_changed); }

    void setup() {
      static const float scale = zincbox::ui_scale();
      static const float font_size = zincbox::settings().interface.font_size;
      static const i32 track_height = theme::config().panel_tracklist.track_height * scale;
      const auto track = db::track_by_id(m_track_id);
      set_nine_slice_margin(8.0f);
      set_height(track_height);

      set_layout("ltr mx:5 my:0 s:5 fill expand");

      label.set_is_drawn(false);

      if (!bg) {
        bg = &add_child<ColorRect>(theme::config().panel_tracklist.track_color_odd);
        bg->set_ignore_parents_layout(true);
      }

      if (!hover) {
        hover = &add_child<ColorRect>(theme::config().panel_tracklist.track_hovered);
        hover->set_opacity(theme::config().panel_tracklist.track_hovered_opacity);
        hover->set_ignore_parents_layout(true);
        hover->set_is_drawn(false);
      }

      if (!label_track_number) {
        label_track_number = &add_child<Label>();
        label_track_number->set_label_anchor(Anchor::LEFT);
        label_track_number->set_text_color(theme::config().panel_tracklist.track_number_color);
      }
      label_track_number->set_text(std::to_string(m_track_number));
      label_track_number->update();
      label_track_number->set_min_width(std::max<i32>(font_size * 1.3f, label_track_number->get_text_extents().x));
      label_track_number->set_max_width(std::max<i32>(font_size * 1.3f, label_track_number->get_text_extents().x));

      if (!label_track_artist) {
        label_track_artist = &add_child<Label>();
        label_track_artist->set_label_anchor(Anchor::LEFT);
        label_track_artist->set_text_color(theme::config().panel_tracklist.track_artist_color);
      }
      if (track.has_value()) {
        if (track->get().metadata.artist.empty() || track->get().metadata.title.empty()) {
          label_track_artist->set_is_drawn(false);
          label_track_artist->set_text("");
          label_track_artist->set_min_width(0);
          label_track_artist->set_max_width(0);
        } else {
          label_track_artist->set_is_drawn(true);
          label_track_artist->set_text(track->get().metadata.artist);
          label_track_artist->update();
          label_track_artist->set_min_width(0);
          label_track_artist->set_max_width(label_track_artist->get_text_extents().x);
        }
      }
      if (!label_track_title) {
        label_track_title = &add_child<Label>();
        label_track_title->set_label_anchor(Anchor::LEFT);
        label_track_title->set_text_color(theme::config().panel_tracklist.title_color);
      }
      if (track.has_value()) {
        label_track_title->set_text(track->get().pretty_title());
        label_track_title->update();
      }

      if (!love_icon) {
        love_icon = &add_child<Sprite>("love");
        love_icon->set_min_width(12);
        love_icon->set_max_width(12);
        love_icon->set_min_height(12);
        love_icon->set_max_height(12);
        love_icon->set_nine_slice_margin(0.0f);
      }
      love_icon->set_is_drawn(db::playlist_loved_tracks().has_track_id(m_track_id));

      if (!label_track_length) {
        label_track_length = &add_child<Label>();
        label_track_length->set_text_color(theme::config().panel_tracklist.length_color);
      }

      if (track.has_value()) { label_track_length->set_text(track->get().pretty_length()); }
      label_track_length->update();
      label_track_length->set_min_width(std::max<i32>(font_size * 2.2f, label_track_length->get_text_extents().x));
      label_track_length->set_max_width(std::max<i32>(font_size * 2.2f, label_track_length->get_text_extents().x));

      if (track.has_value() && track->get().get_flag(db::TOMBSTONE)) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      }

      set_playback_error(track.has_value() && track->get().get_flag(db::PLAYBACK_ERROR));
      update_text_colors();
    }

    void set_highlighted(bool highlighted_new) {
      if (m_highlighted == highlighted_new) { return; }
      m_highlighted = highlighted_new;

      update_text_colors();
    }

    void set_playback_error(bool playback_error_new) {
      if (m_playback_error == playback_error_new) { return; }
      m_playback_error = playback_error_new;

      update_text_colors();
    }

    void update_text_colors() {
      std::string txt;
      if (m_is_selected) {
        if (m_track_number % 2 == 0) {
          bg->set_color(theme::config().panel_tracklist.track_color_selected_odd);
        } else {
          bg->set_color(theme::config().panel_tracklist.track_color_selected_even);
        }
      } else if (m_highlighted) {
        bg->set_color(theme::config().panel_tracklist.track_color_playing);
      } else {
        if (m_track_number % 2 == 0) {
          bg->set_color(theme::config().panel_tracklist.track_color_odd);
        } else {
          bg->set_color(theme::config().panel_tracklist.track_color_even);
        }
      }

      label_track_number->set_text_color(theme::config().panel_tracklist.track_number_color);
      label_track_artist->set_text_color(theme::config().panel_tracklist.track_artist_color);
      label_track_title->set_text_color(theme::config().panel_tracklist.title_color);
      label_track_length->set_text_color(theme::config().panel_tracklist.length_color);

      auto track = db::track_by_id(m_track_id);
      if (track.has_value()) {
        if (m_is_selected) {
          label_track_number->set_text_color(label_track_number->get_text_color() * 5.0f);
          label_track_artist->set_text_color(label_track_artist->get_text_color() * 5.0f);
          label_track_title->set_text_color(label_track_title->get_text_color() * 5.0f);
          label_track_length->set_text_color(label_track_length->get_text_color() * 5.0f);
        } else if (track->get().get_flag(db::TOMBSTONE) || m_playback_error) {
          label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
          label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
          label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
          label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
        } else if (m_highlighted) {
          label_track_number->set_text_color(label_track_number->get_text_color() * 1.5f);
          label_track_artist->set_text_color(label_track_artist->get_text_color() * 1.5f);
          label_track_title->set_text_color(label_track_title->get_text_color() * 1.5f);
          label_track_length->set_text_color(label_track_length->get_text_color() * 1.5f);
        }
      }
    }

    void update() override {
      if (m_changed) {
        setup();
        update_highlight_status_from_player();
        m_changed = false;
      }

      if (!is_mouse_hovering()) { m_is_hovered = false; }

      hover->set_width(width);
      hover->set_height(height);
      hover->set_is_drawn(m_is_hovered);

      bg->set_width(width);
      bg->set_height(height);

      Sprite::update();
    }

    void event(Input::InputEventMouseMove& ev) override {
      m_is_hovered = is_mouse_hovering();
      Button::event(ev);
    }

  public:
    WidgetTrack& track_id(size_t value) {
      if (m_track_id != value) {
        m_track_id = value;
        m_changed = true;
      }
      return *this;
    }
    WidgetTrack& playlist_id(size_t value) {
      if (m_playlist_id != value) {
        m_playlist_id = value;
        m_changed = true;
      }
      return *this;
    }
    WidgetTrack& collection_id(size_t value) {
      if (m_collection_id != value) {
        m_collection_id = value;
        m_changed = true;
      }
      return *this;
    }
    WidgetTrack& track_number(size_t value) {
      if (m_track_number != value) {
        m_track_number = value;
        m_changed = true;
      }
      return *this;
    }
    WidgetTrack& is_selected(bool value) {
      if (m_is_selected != value) {
        m_is_selected = value;
        m_changed = true;
      }
      return *this;
    }

    WidgetTrack& highlight_mode(TrackHighlightMode value) {
      if (m_highlight_mode != value) {
        m_highlight_mode = value;
        m_changed = true;
      }
      return *this;
    }

    size_t track_id() const { return m_track_id; }
    size_t playlist_id() const { return m_playlist_id; }
    size_t collection_id() const { return m_collection_id; }
    size_t track_number() const { return m_track_number; }
    size_t is_selected() const { return m_is_selected; }
    db::track_info track_info() const {
      return db::track_info{.collection_id = m_collection_id,
                            .playlist_id = m_playlist_id,
                            .track_id = m_track_id,
                            .index = m_track_number - 1};
    }

  protected:
    bool m_is_hovered = false;
    size_t m_track_id = db::INVALID_ID;
    size_t m_playlist_id = db::INVALID_ID;
    size_t m_collection_id = db::INVALID_ID;
    size_t m_track_number = 0;
    bool m_is_selected = false;
    TrackHighlightMode m_highlight_mode = TrackHighlightMode::TRACK_INFO;
    bool m_changed = true;
    Signal<>::slot_key slot_on_track_changed;
    Label* label_track_number{};
    Label* label_track_artist{};
    Label* label_track_title{};
    Label* label_track_length{};
    ColorRect* bg{};
    ColorRect* hover{};
    Sprite* love_icon{};
    bool m_highlighted = false;
    bool m_playback_error = false;
    bool pressed = false;
};
