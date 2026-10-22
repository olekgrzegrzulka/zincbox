#pragma once
#include <algorithm>
#include <string>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "core/settings.hpp"
#include "theme.hpp"
#include "ui/cover_art.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

class WidgetQueuePlaylistCollection final : public zincgui::Button {
  public:
    enum class ItemType : u8 { PLAYLIST, COLLECTION };

    WidgetQueuePlaylistCollection(const WidgetQueuePlaylistCollection&) = default;
    WidgetQueuePlaylistCollection(WidgetQueuePlaylistCollection&&) = delete;
    WidgetQueuePlaylistCollection& operator=(const WidgetQueuePlaylistCollection&) = delete;
    WidgetQueuePlaylistCollection& operator=(WidgetQueuePlaylistCollection&&) = delete;
    WidgetQueuePlaylistCollection(zincgui::Root& ui_) : Button(ui_) {
      setup();

      m_slot_on_track_changed =
        player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
    }

    void update_highlight_status_from_player() {
      set_highlighted(player::get_playing_index().has_value() && m_queue_index == player::get_playing_index().value());
    }

    ~WidgetQueuePlaylistCollection() override { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

    void setup() {
      const float SCALE = zincbox::ui_scale();
      const float FONT_SIZE = zincbox::settings().interface.font_size;
      i32 item_height = m_item_type == ItemType::PLAYLIST ? theme::config().panel_queue.playlist_height * SCALE
                                                          : theme::config().panel_queue.collection_height * SCALE;

      set_nine_slice_margin(8.0f);
      set_height(item_height);
      set_layout("ltr mx:5 my:0 s:5 middle fill expand");

      label.set_is_drawn(false);
      label.set_ignore_parents_layout(true);

      if (!m_bg) {
        m_bg = &add_child<zincgui::ColorRect>(theme::config().panel_queue.track_color_odd);
        m_bg->set_ignore_parents_layout(true);
      }

      if (!m_hover) {
        m_hover = &add_child<zincgui::ColorRect>(theme::config().panel_queue.track_hovered);
        m_hover->set_opacity(theme::config().panel_queue.track_hovered_opacity);
        m_hover->set_ignore_parents_layout(true);
      }

      if (!m_label_queue_index) {
        m_label_queue_index = &add_child<zincgui::Label>();
        m_label_queue_index->set_label_anchor(zincgui::Anchor::LEFT);
      }
      m_label_queue_index->set_text(std::to_string(m_queue_index + 1));
      m_label_queue_index->set_min_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_queue_index->get_text_extents().x));
      m_label_queue_index->set_max_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_queue_index->get_text_extents().x));

      if (!m_cover_art) {
        m_cover_art = &add_child<CoverArt>();
        i32 cover_size = item_height - (10 * SCALE);
        m_cover_art->set_min_width(cover_size);
        m_cover_art->set_max_width(cover_size);
        m_cover_art->set_fixed_height(cover_size);
        m_cover_art->set_nine_slice_margin(0.0f);
        m_cover_art->set_atlas_index(2);
        m_cover_art->set_is_drawn(false);
      }

      if (!m_text_container) {
        m_text_container = &add_child<zincgui::Widget>();
        m_text_container->set_layout("ttb mx:0 my:0 s:0 fill expand");

        m_label_top = &m_text_container->add_child<zincgui::Label>();
        m_label_top->set_label_anchor(zincgui::Anchor::BOTTOM_LEFT);

        m_label_bottom = &m_text_container->add_child<zincgui::Label>();
        m_label_bottom->set_label_anchor(zincgui::Anchor::TOP_LEFT);
      }

      std::string length_str;

      if (m_item_type == ItemType::PLAYLIST) {
        auto pl = db::playlist_by_id(m_playlist_id);
        if (pl.has_value()) {
          std::string artist = pl->get().author_pretty();
          if (artist.empty()) {
            m_label_top->set_text(pl->get().name);
            m_label_top->set_label_anchor(zincgui::Anchor::LEFT);
            m_label_bottom->set_is_drawn(false);
          } else {
            m_label_top->set_text(artist);
            m_label_top->set_label_anchor(zincgui::Anchor::LEFT);
            m_label_bottom->set_text(pl->get().name);
            m_label_bottom->set_is_drawn(true);
          }
          length_str = pl->get().pretty_length();
        }
      } else {
        auto col = db::collection_by_id(m_collection_id);
        if (col.has_value()) {
          m_label_top->set_text(col->get().name());
          m_label_bottom->set_is_drawn(false);
          length_str = col->get().pretty_length();
        }
      }

      if (!m_label_progress) {
        m_label_progress = &add_child<zincgui::Label>();
        m_label_progress->set_label_anchor(zincgui::Anchor::RIGHT);
      }
      m_label_progress->set_text(std::to_string(m_track_current + 1) + " / " + std::to_string(m_tracks_total));

      if (!m_label_length) {
        m_label_length = &add_child<zincgui::Label>();
        m_label_length->set_label_anchor(zincgui::Anchor::RIGHT);
      }

      m_label_length->set_text(length_str);
      m_label_length->set_min_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_length->get_text_extents().x));
      m_label_length->set_max_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_length->get_text_extents().x));

      update_text_colors();
    }

    void set_highlighted(bool highlighted_new) {
      if (m_highlighted == highlighted_new) { return; }
      m_highlighted = highlighted_new;

      update_text_colors();
    }

    void update_text_colors() {
      if (m_is_selected) {
        if ((m_queue_index + 1) % 2 == 0) {
          m_bg->set_color(theme::config().panel_queue.track_color_selected_odd);
        } else {
          m_bg->set_color(theme::config().panel_queue.track_color_selected_even);
        }
      } else if (m_highlighted) {
        m_bg->set_color(theme::config().panel_queue.track_color_playing);
      } else {
        if ((m_queue_index + 1) % 2 == 0) {
          m_bg->set_color(theme::config().panel_queue.track_color_odd);
        } else {
          m_bg->set_color(theme::config().panel_queue.track_color_even);
        }
      }

      m_label_queue_index->set_text_color(theme::config().panel_queue.track_number_color);
      m_label_length->set_text_color(theme::config().panel_queue.length_color);

      if (m_item_type == ItemType::PLAYLIST) {
        auto pl = db::playlist_by_id(m_playlist_id);
        bool has_artist = pl.has_value() && !pl->get().author_pretty().empty();
        if (has_artist) {
          m_label_top->set_text_color(theme::config().panel_queue.track_artist_color);
          m_label_bottom->set_text_color(theme::config().panel_queue.title_color);
        } else {
          m_label_top->set_text_color(theme::config().panel_queue.title_color);
        }
      } else {
        m_label_top->set_text_color(theme::config().panel_queue.title_color);
      }

      if (m_is_selected) {
        m_label_queue_index->set_text_color(m_label_queue_index->get_text_color() * 5.0f);
        m_label_top->set_text_color(m_label_top->get_text_color() * 5.0f);
        m_label_bottom->set_text_color(m_label_bottom->get_text_color() * 5.0f);
        m_label_length->set_text_color(m_label_length->get_text_color() * 5.0f);
      } else if (m_highlighted) {
        m_label_queue_index->set_text_color(m_label_queue_index->get_text_color() * 1.5f);
        m_label_top->set_text_color(m_label_top->get_text_color() * 1.5f);
        m_label_bottom->set_text_color(m_label_bottom->get_text_color() * 1.5f);
        m_label_length->set_text_color(m_label_length->get_text_color() * 1.5f);
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

      Button::update();
    }

    void event(zincgui::Input::InputEventMouseMove& ev) override {
      m_is_hovered = is_mouse_hovering();
      Button::event(ev);
    }

  public:
    WidgetQueuePlaylistCollection& playlist_id(size_t value) {
      if (m_playlist_id != value) {
        m_playlist_id = value;
        m_changed = true;

        if (m_item_type == ItemType::PLAYLIST) {
          auto cover = zincbox::ui::cover_art::get_texture_id_of_playlist(m_playlist_id);
          if (cover) { m_cover_art->set_texture(*cover); }
          m_cover_art->set_is_drawn(true);
        }
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& collection_id(size_t value) {
      if (m_collection_id != value) {
        m_collection_id = value;
        m_changed = true;

        if (m_item_type == ItemType::COLLECTION) {
          auto cover = zincbox::ui::cover_art::get_texture_id_of_collection(m_collection_id);
          if (cover) { m_cover_art->set_texture(*cover); }
          m_cover_art->set_is_drawn(true);
        }
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& queue_index(size_t value) {
      if (m_queue_index != value) {
        m_queue_index = value;
        m_changed = true;
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& track_current(size_t value) {
      if (m_track_current != value) {
        m_track_current = value;
        m_changed = true;
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& tracks_total(size_t value) {
      if (m_tracks_total != value) {
        m_tracks_total = value;
        m_changed = true;
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& is_selected(bool value) {
      if (m_is_selected != value) {
        m_is_selected = value;
        m_changed = true;
      }
      return *this;
    }

    WidgetQueuePlaylistCollection& item_type(ItemType value) {
      if (m_item_type != value) {
        m_item_type = value;
        m_changed = true;
      }
      return *this;
    }

    size_t playlist_id() const { return m_playlist_id; }
    size_t collection_id() const { return m_collection_id; }
    size_t queue_index() const { return m_queue_index; }
    size_t track_current() const { return m_track_current; }
    size_t tracks_total() const { return m_tracks_total; }
    bool is_selected() const { return m_is_selected; }
    ItemType item_type() const { return m_item_type; }

  private:
    bool m_is_hovered = false;
    size_t m_playlist_id = db::INVALID_ID;
    size_t m_collection_id = db::INVALID_ID;
    size_t m_queue_index = 0;
    size_t m_track_current = 0;
    size_t m_tracks_total = 0;
    bool m_is_selected = false;
    ItemType m_item_type = ItemType::PLAYLIST;
    bool m_changed = true;

    class CoverArt : public zincgui::Sprite {
      public:
        CoverArt(class zincgui::Root& root_) : zincgui::Sprite(root_) {}
        zincgui::TextureAtlas& get_texture_atlas() override { return zincbox::ui::cover_art::texture_atlas(); }
    };

    Signal<>::slot_key m_slot_on_track_changed;
    zincgui::ColorRect* m_bg{};
    zincgui::ColorRect* m_hover{};
    zincgui::Label* m_label_queue_index{};
    CoverArt* m_cover_art{};
    zincgui::Widget* m_text_container{};
    zincgui::Label* m_label_top{};
    zincgui::Label* m_label_bottom{};
    zincgui::Label* m_label_progress{};
    zincgui::Label* m_label_length{};
    bool m_highlighted = false;
};
