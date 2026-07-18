#pragma once
#include <string>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
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
      static const i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
      const auto track = db::track_by_id(m_track_id);
      const std::string txt = m_track_number % 2 == 0 ? "track_bg2" : "track_bg1";

      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      set_texture(txt, false);
      set_nine_slice_margin(8.0f);
      set_height(track_height);

      set_layout("ltr mx:5 my:0 s:5 fill expand");

      label.set_is_drawn(false);

      if (!label_track_number) {
        label_track_number = &add_child<Label>();
        label_track_number->set_label_anchor(Anchor::LEFT);
        label_track_number->set_text_color(theme::get_prop("tracklist_track_number_text_color").as_rgba());
      }
      label_track_number->set_text(std::to_string(m_track_number));
      label_track_number->update();
      label_track_number->set_min_width(std::max<i32>(18, label_track_number->get_text_extents().x));
      label_track_number->set_max_width(std::max<i32>(18, label_track_number->get_text_extents().x));

      if (!label_track_artist) {
        label_track_artist = &add_child<Label>();
        label_track_artist->set_label_anchor(Anchor::LEFT);
        label_track_artist->set_text_color(theme::get_prop("tracklist_artist_text_color").as_rgba());
      }
      if (track.has_value()) {
        if (track->get().artist.empty() || track->get().title.empty()) {
          label_track_artist->set_is_drawn(false);
          label_track_artist->set_text("");
          label_track_artist->set_min_width(0);
          label_track_artist->set_max_width(0);
        } else {
          label_track_artist->set_is_drawn(true);
          label_track_artist->set_text(track->get().artist);
          label_track_artist->update();
          label_track_artist->set_min_width(0);
          label_track_artist->set_max_width(label_track_artist->get_text_extents().x);
        }
      }
      if (!label_track_title) {
        label_track_title = &add_child<Label>();
        label_track_title->set_label_anchor(Anchor::LEFT);
        label_track_title->set_text_color(theme::get_prop("tracklist_title_text_color").as_rgba());
      }
      if (track.has_value()) {
        if (track->get().artist.empty() || track->get().title.empty()) {
          label_track_title->set_text(std::filesystem::path{track->get().path}.filename().string());
        } else {
          label_track_title->set_text(track->get().title);
        }
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
        label_track_length->set_text_color(theme::get_prop("tracklist_length_text_color").as_rgba());
      }

      if (track.has_value()) { label_track_length->set_text(track->get().pretty_length()); }
      label_track_length->update();
      label_track_length->set_min_width(std::max<i32>(32, label_track_length->get_text_extents().x));
      label_track_length->set_max_width(std::max<i32>(32, label_track_length->get_text_extents().x));

      if (track.has_value() && track->get().is_tombstone()) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      }

      if (!hover) {
        hover = &add_child<Sprite>("track_hovered");
        hover->set_ignore_parents_layout(true);
      }

      set_playback_error(track.has_value() && track->get().is_playback_error());
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
        txt = m_track_number % 2 == 0 ? "track_bg_selected2" : "track_bg_selected1";
      } else if (m_highlighted) {
        txt = "track_bg_playing";
      } else {
        txt = m_track_number % 2 == 0 ? "track_bg2" : "track_bg1";
      }

      set_texture(txt, false);
      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      label_track_number->set_text_color(theme::get_prop("tracklist_track_number_text_color").as_rgba());
      label_track_artist->set_text_color(theme::get_prop("tracklist_artist_text_color").as_rgba());
      label_track_title->set_text_color(theme::get_prop("tracklist_title_text_color").as_rgba());
      label_track_length->set_text_color(theme::get_prop("tracklist_length_text_color").as_rgba());

      auto track = db::track_by_id(m_track_id);
      if (track.has_value()) {
        if (m_is_selected) {
          label_track_number->set_text_color(label_track_number->get_text_color() * 5.0f);
          label_track_artist->set_text_color(label_track_artist->get_text_color() * 5.0f);
          label_track_title->set_text_color(label_track_title->get_text_color() * 5.0f);
          label_track_length->set_text_color(label_track_length->get_text_color() * 5.0f);
        } else if (track->get().is_tombstone() || m_playback_error) {
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
    Sprite* hover{};
    Sprite* love_icon{};
    bool m_highlighted = false;
    bool m_playback_error = false;
    bool pressed = false;
};
