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

    ~WidgetQueuePlaylistCollection() override { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

    void update() override {
      if (m_changed) {
        setup();
        update_highlight_status_from_player();
        m_changed = false;
      }

      if (!is_mouse_hovering()) { m_is_hovered = false; }

      m_hover->size(width(), height()).is_drawn(m_is_hovered);

      m_bg->size(width(), height());

      Button::update();
    }

    void event(zincgui::Input::InputEventMouseMove& ev) override {
      m_is_hovered = is_mouse_hovering();
      Button::event(ev);
    }

    void update_highlight_status_from_player() {
      highlighted(player::get_playing_index().has_value() && m_queue_index == player::get_playing_index().value());
    }

    void setup() {
      const float SCALE = zincbox::ui_scale();
      const float FONT_SIZE = zincbox::settings().interface.font_size;
      i32 item_height = m_item_type == ItemType::PLAYLIST ? theme::config().panel_queue.playlist_height * SCALE
                                                          : theme::config().panel_queue.collection_height * SCALE;

      nine_slice_margin(8.0f).height(item_height).layout("ltr mx:5 my:0 s:5 middle fill expand");

      label().is_drawn(false).ignore_parents_layout(true);

      if (!m_bg) {
        m_bg = &add_child<zincgui::ColorRect>(theme::config().panel_queue.track_color_odd);
        m_bg->ignore_parents_layout(true);
      }

      if (!m_hover) {
        m_hover = &add_child<zincgui::ColorRect>(theme::config().panel_queue.track_hovered);
        m_hover->opacity(theme::config().panel_queue.track_hovered_opacity).ignore_parents_layout(true);
      }

      if (!m_label_queue_index) {
        m_label_queue_index = &add_child<zincgui::Label>();
        m_label_queue_index->label_anchor(zincgui::Anchor::LEFT);
      }
      m_label_queue_index->text(std::to_string(m_queue_index + 1))
        .min_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_queue_index->text_extents().x))
        .max_width(std::max<i32>(FONT_SIZE * 1.3f, m_label_queue_index->text_extents().x));

      if (!m_cover_art) {
        m_cover_art = &add_child<CoverArt>();
        i32 cover_size = item_height - (10 * SCALE);
        m_cover_art->min_width(cover_size)
          .max_width(cover_size)
          .fixed_height(cover_size)
          .nine_slice_margin(0.0f)
          .atlas_index(2)
          .is_drawn(false);
      }

      if (!m_text_container) {
        m_text_container = &add_child<zincgui::Widget>();
        m_text_container->layout("ttb mx:0 my:0 s:0 fill expand");

        m_label_top = &m_text_container->add_child<zincgui::Label>();
        m_label_top->label_anchor(zincgui::Anchor::BOTTOM_LEFT);

        m_label_bottom = &m_text_container->add_child<zincgui::Label>();
        m_label_bottom->label_anchor(zincgui::Anchor::TOP_LEFT);
      }

      std::string length_str;

      if (m_item_type == ItemType::PLAYLIST) {
        auto pl = db::playlist_by_id(m_playlist_id);
        if (pl.has_value()) {
          std::string artist = pl->get().author_pretty();
          if (artist.empty()) {
            m_label_top->text(pl->get().name).label_anchor(zincgui::Anchor::LEFT);
            m_label_bottom->is_drawn(false);
          } else {
            m_label_top->text(artist).label_anchor(zincgui::Anchor::LEFT);
            m_label_bottom->text(pl->get().name).is_drawn(true);
          }
          length_str = pl->get().pretty_length();
        }
      } else {
        auto col = db::collection_by_id(m_collection_id);
        if (col.has_value()) {
          m_label_top->text(col->get().name());
          m_label_bottom->is_drawn(false);
          length_str = col->get().pretty_length();
        }
      }

      if (!m_label_progress) {
        m_label_progress = &add_child<zincgui::Label>();
        m_label_progress->label_anchor(zincgui::Anchor::RIGHT);
      }
      m_label_progress->text(std::to_string(m_track_current + 1) + " / " + std::to_string(m_tracks_total));

      if (!m_label_length) {
        m_label_length = &add_child<zincgui::Label>();
        m_label_length->label_anchor(zincgui::Anchor::RIGHT);
      }

      m_label_length->text(length_str)
        .min_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_length->text_extents().x))
        .max_width(std::max<i32>(FONT_SIZE * 2.6f, m_label_length->text_extents().x));

      update_text_colors();
    }

    void update_text_colors() {
      if (m_is_selected) {
        if ((m_queue_index + 1) % 2 == 0) {
          m_bg->color(theme::config().panel_queue.track_color_selected_odd);
        } else {
          m_bg->color(theme::config().panel_queue.track_color_selected_even);
        }
      } else if (m_highlighted) {
        m_bg->color(theme::config().panel_queue.track_color_playing);
      } else {
        if ((m_queue_index + 1) % 2 == 0) {
          m_bg->color(theme::config().panel_queue.track_color_odd);
        } else {
          m_bg->color(theme::config().panel_queue.track_color_even);
        }
      }

      m_label_queue_index->text_color(theme::config().panel_queue.track_number_color);
      m_label_length->text_color(theme::config().panel_queue.length_color);

      if (m_item_type == ItemType::PLAYLIST) {
        auto pl = db::playlist_by_id(m_playlist_id);
        bool has_artist = pl.has_value() && !pl->get().author_pretty().empty();
        if (has_artist) {
          m_label_top->text_color(theme::config().panel_queue.track_artist_color);
          m_label_bottom->text_color(theme::config().panel_queue.title_color);
        } else {
          m_label_top->text_color(theme::config().panel_queue.title_color);
        }
      } else {
        m_label_top->text_color(theme::config().panel_queue.title_color);
      }

      if (m_is_selected) {
        m_label_queue_index->text_color(m_label_queue_index->text_color() * 5.0f);
        m_label_top->text_color(m_label_top->text_color() * 5.0f);
        m_label_bottom->text_color(m_label_bottom->text_color() * 5.0f);
        m_label_length->text_color(m_label_length->text_color() * 5.0f);
      } else if (m_highlighted) {
        m_label_queue_index->text_color(m_label_queue_index->text_color() * 1.5f);
        m_label_top->text_color(m_label_top->text_color() * 1.5f);
        m_label_bottom->text_color(m_label_bottom->text_color() * 1.5f);
        m_label_length->text_color(m_label_length->text_color() * 1.5f);
      }
    }

    WidgetQueuePlaylistCollection& playlist_id(size_t value) {
      if (m_playlist_id != value) {
        m_playlist_id = value;
        m_changed = true;

        if (m_item_type == ItemType::PLAYLIST) {
          auto cover = zincbox::ui::cover_art::get_texture_id_of_playlist(m_playlist_id);
          if (cover) { m_cover_art->texture(*cover); }
          m_cover_art->is_drawn(true);
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
          if (cover) { m_cover_art->texture(*cover); }
          m_cover_art->is_drawn(true);
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

    WidgetQueuePlaylistCollection& highlighted(bool highlighted_new) {
      if (m_highlighted == highlighted_new) { return *this; }
      m_highlighted = highlighted_new;
      update_text_colors();
      return *this;
    }

    [[nodiscard]] size_t playlist_id() const noexcept { return m_playlist_id; }
    [[nodiscard]] size_t collection_id() const noexcept { return m_collection_id; }
    [[nodiscard]] size_t queue_index() const noexcept { return m_queue_index; }
    [[nodiscard]] size_t track_current() const noexcept { return m_track_current; }
    [[nodiscard]] size_t tracks_total() const noexcept { return m_tracks_total; }
    [[nodiscard]] bool is_selected() const noexcept { return m_is_selected; }
    [[nodiscard]] ItemType item_type() const noexcept { return m_item_type; }
    [[nodiscard]] bool highlighted() const noexcept { return m_highlighted; }

    [[nodiscard]] zincgui::ColorRect* bg() noexcept { return m_bg; }
    [[nodiscard]] const zincgui::ColorRect* bg() const noexcept { return m_bg; }
    [[nodiscard]] zincgui::ColorRect* hover() noexcept { return m_hover; }
    [[nodiscard]] const zincgui::ColorRect* hover() const noexcept { return m_hover; }
    [[nodiscard]] zincgui::Label* label_queue_index() noexcept { return m_label_queue_index; }
    [[nodiscard]] const zincgui::Label* label_queue_index() const noexcept { return m_label_queue_index; }
    [[nodiscard]] zincgui::Sprite* cover_art() noexcept { return m_cover_art; }
    [[nodiscard]] const zincgui::Sprite* cover_art() const noexcept { return m_cover_art; }
    [[nodiscard]] zincgui::Widget* text_container() noexcept { return m_text_container; }
    [[nodiscard]] const zincgui::Widget* text_container() const noexcept { return m_text_container; }
    [[nodiscard]] zincgui::Label* label_top() noexcept { return m_label_top; }
    [[nodiscard]] const zincgui::Label* label_top() const noexcept { return m_label_top; }
    [[nodiscard]] zincgui::Label* label_bottom() noexcept { return m_label_bottom; }
    [[nodiscard]] const zincgui::Label* label_bottom() const noexcept { return m_label_bottom; }
    [[nodiscard]] zincgui::Label* label_progress() noexcept { return m_label_progress; }
    [[nodiscard]] const zincgui::Label* label_progress() const noexcept { return m_label_progress; }
    [[nodiscard]] zincgui::Label* label_length() noexcept { return m_label_length; }
    [[nodiscard]] const zincgui::Label* label_length() const noexcept { return m_label_length; }

  private:
    class CoverArt : public zincgui::Sprite {
      public:
        CoverArt(zincgui::Root& root_) : zincgui::Sprite(root_) {}
        [[nodiscard]] zincgui::TextureAtlas& texture_atlas() override {
          return zincbox::ui::cover_art::texture_atlas();
        }
        [[nodiscard]] zincgui::TextureAtlas& get_texture_atlas() { return texture_atlas(); }
    };

    bool m_is_hovered = false;
    size_t m_playlist_id = db::INVALID_ID;
    size_t m_collection_id = db::INVALID_ID;
    size_t m_queue_index = 0;
    size_t m_track_current = 0;
    size_t m_tracks_total = 0;
    bool m_is_selected = false;
    ItemType m_item_type = ItemType::PLAYLIST;
    bool m_changed = true;
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
