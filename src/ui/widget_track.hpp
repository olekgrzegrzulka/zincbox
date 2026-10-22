#pragma once
#include <string>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "core/settings.hpp"
#include "theme.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/root.hpp"
#include "zincbox.hpp"

class WidgetTrack final : public zincgui::Button {
  public:
    enum class TrackHighlightMode : u8 { TRACK_INFO, QUEUE_INDEX, OFF };
    WidgetTrack(const WidgetTrack&) = delete;
    WidgetTrack(WidgetTrack&&) = delete;
    WidgetTrack& operator=(const WidgetTrack&) = delete;
    WidgetTrack& operator=(WidgetTrack&&) = delete;
    WidgetTrack(zincgui::Root& ui_) : Button(ui_) {
      setup();

      m_slot_on_track_changed =
        player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
    }

    void update_highlight_status_from_player() {
      if (m_highlight_mode == TrackHighlightMode::QUEUE_INDEX) {
        set_highlighted(player::get_playing_index() == m_track_number - 1);
      } else if (m_highlight_mode == TrackHighlightMode::TRACK_INFO) {
        set_highlighted(player::get_playing_track() && m_track_id == player::get_playing_track()->track_id &&
                        m_playlist_id == player::get_playing_track()->playlist_id &&
                        m_collection_id == player::get_playing_track()->collection_id);
      }
    }

    ~WidgetTrack() override { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

    void setup() {
      const float SCALE = zincbox::ui_scale();
      const float FONT_SIZE = zincbox::settings().interface.font_size;
      const i32 TRACK_HEIGHT = theme::config().panel_tracklist.track_height * SCALE;
      auto track = db::track_by_id(m_track_id);
      set_nine_slice_margin(8.0f);
      set_height(TRACK_HEIGHT);

      set_layout("ltr mx:5 my:0 s:5 fill expand");

      label.set_is_drawn(false);

      if (!m_bg) {
        m_bg = &add_child<zincgui::ColorRect>(theme::config().panel_tracklist.track_color_odd);
        m_bg->set_ignore_parents_layout(true);
      }

      if (!m_hover) {
        m_hover = &add_child<zincgui::ColorRect>(theme::config().panel_tracklist.track_hovered);
        m_hover->set_opacity(theme::config().panel_tracklist.track_hovered_opacity);
        m_hover->set_ignore_parents_layout(true);
        m_hover->set_is_drawn(false);
      }

      if (!m_label_track_number) {
        m_label_track_number = &add_child<zincgui::Label>();
        m_label_track_number->set_label_anchor(zincgui::Anchor::LEFT);
        m_label_track_number->set_text_color(theme::config().panel_tracklist.track_number_color);
      }
      m_label_track_number->set_text(std::to_string(m_track_number));
      m_label_track_number->set_min_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_track_number->get_text_extents().x));
      m_label_track_number->set_max_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_track_number->get_text_extents().x));

      if (!m_label_track_artist) {
        m_label_track_artist = &add_child<zincgui::Label>();
        m_label_track_artist->set_label_anchor(zincgui::Anchor::LEFT);
        m_label_track_artist->set_text_color(theme::config().panel_tracklist.track_artist_color);
      }
      if (track.has_value()) {
        if (track->get().metadata.artist.empty() || track->get().metadata.title.empty()) {
          m_label_track_artist->set_is_drawn(false);
          m_label_track_artist->set_text("");
          m_label_track_artist->set_min_width(0);
          m_label_track_artist->set_max_width(0);
        } else {
          m_label_track_artist->set_is_drawn(true);
          m_label_track_artist->set_text(track->get().metadata.artist);
          m_label_track_artist->set_min_width(0);
          m_label_track_artist->set_max_width(m_label_track_artist->get_text_extents().x);
        }
      }
      if (!m_label_track_title) {
        m_label_track_title = &add_child<zincgui::Label>();
        m_label_track_title->set_label_anchor(zincgui::Anchor::LEFT);
        m_label_track_title->set_text_color(theme::config().panel_tracklist.title_color);
      }
      if (track.has_value()) { m_label_track_title->set_text(track->get().pretty_title()); }

      if (!m_love_icon) {
        m_love_icon = &add_child<Sprite>("love");
        m_love_icon->set_min_width(m_love_icon->get_width());
        m_love_icon->set_max_width(m_love_icon->get_width());
        m_love_icon->set_fixed_height(m_love_icon->get_height());
        m_love_icon->set_nine_slice_margin(0.0f);
      }
      m_love_icon->set_is_drawn(db::playlist_loved_tracks().has_track_id(m_track_id));

      if (!m_label_track_length) {
        m_label_track_length = &add_child<zincgui::Label>();
        m_label_track_length->set_label_anchor(zincgui::Anchor::RIGHT);
        m_label_track_length->set_text_color(theme::config().panel_tracklist.length_color);
      }

      if (track.has_value()) { m_label_track_length->set_text(track->get().pretty_length()); }
      m_label_track_length->set_min_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_track_length->get_text_extents().x));
      m_label_track_length->set_max_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_track_length->get_text_extents().x));

      if (track.has_value() && track->get().get_flag(db::TOMBSTONE)) {
        m_label_track_number->set_text_color(m_label_track_number->get_text_color() * 0.6f);
        m_label_track_artist->set_text_color(m_label_track_artist->get_text_color() * 0.6f);
        m_label_track_title->set_text_color(m_label_track_title->get_text_color() * 0.6f);
        m_label_track_length->set_text_color(m_label_track_length->get_text_color() * 0.6f);
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
      if (m_is_selected) {
        if (m_track_number % 2 == 0) {
          m_bg->set_color(theme::config().panel_tracklist.track_color_selected_odd);
        } else {
          m_bg->set_color(theme::config().panel_tracklist.track_color_selected_even);
        }
      } else if (m_highlighted) {
        m_bg->set_color(theme::config().panel_tracklist.track_color_playing);
      } else {
        if (m_track_number % 2 == 0) {
          m_bg->set_color(theme::config().panel_tracklist.track_color_odd);
        } else {
          m_bg->set_color(theme::config().panel_tracklist.track_color_even);
        }
      }

      m_label_track_number->set_text_color(theme::config().panel_tracklist.track_number_color);
      m_label_track_artist->set_text_color(theme::config().panel_tracklist.track_artist_color);
      m_label_track_title->set_text_color(theme::config().panel_tracklist.title_color);
      m_label_track_length->set_text_color(theme::config().panel_tracklist.length_color);

      auto track = db::track_by_id(m_track_id);
      if (track.has_value()) {
        if (m_is_selected) {
          m_label_track_number->set_text_color(m_label_track_number->get_text_color() * 5.0f);
          m_label_track_artist->set_text_color(m_label_track_artist->get_text_color() * 5.0f);
          m_label_track_title->set_text_color(m_label_track_title->get_text_color() * 5.0f);
          m_label_track_length->set_text_color(m_label_track_length->get_text_color() * 5.0f);
        } else if (track->get().get_flag(db::TOMBSTONE) || m_playback_error) {
          m_label_track_number->set_text_color(m_label_track_number->get_text_color() * 0.6f);
          m_label_track_artist->set_text_color(m_label_track_artist->get_text_color() * 0.6f);
          m_label_track_title->set_text_color(m_label_track_title->get_text_color() * 0.6f);
          m_label_track_length->set_text_color(m_label_track_length->get_text_color() * 0.6f);
        } else if (m_highlighted) {
          m_label_track_number->set_text_color(m_label_track_number->get_text_color() * 1.5f);
          m_label_track_artist->set_text_color(m_label_track_artist->get_text_color() * 1.5f);
          m_label_track_title->set_text_color(m_label_track_title->get_text_color() * 1.5f);
          m_label_track_length->set_text_color(m_label_track_length->get_text_color() * 1.5f);
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

      m_hover->set_width(width);
      m_hover->set_height(height);
      m_hover->set_is_drawn(m_is_hovered);

      m_bg->set_width(width);
      m_bg->set_height(height);

      Sprite::update();
    }

    void event(zincgui::Input::InputEventMouseMove& ev) override {
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
    WidgetTrack& item_index(size_t value) {
      if (m_item_index != value) {
        m_item_index = value;
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
    bool is_selected() const { return m_is_selected; }
    size_t item_index() const { return m_item_index; }
    db::track_info track_info() const {
      return db::track_info{.collection_id = m_collection_id,
                            .playlist_id = m_playlist_id,
                            .track_id = m_track_id,
                            .index = m_track_number - 1};
    }

  private:
    bool m_is_hovered = false;
    size_t m_track_id = db::INVALID_ID;
    size_t m_playlist_id = db::INVALID_ID;
    size_t m_collection_id = db::INVALID_ID;
    size_t m_track_number = 0;
    bool m_is_selected = false;
    size_t m_item_index = 0;
    TrackHighlightMode m_highlight_mode = TrackHighlightMode::TRACK_INFO;
    bool m_changed = true;
    Signal<>::slot_key m_slot_on_track_changed;
    zincgui::Label* m_label_track_number{};
    zincgui::Label* m_label_track_artist{};
    zincgui::Label* m_label_track_title{};
    zincgui::Label* m_label_track_length{};
    zincgui::ColorRect* m_bg{};
    zincgui::ColorRect* m_hover{};
    Sprite* m_love_icon{};
    bool m_highlighted = false;
    bool m_playback_error = false;
    bool m_pressed = false;
};
