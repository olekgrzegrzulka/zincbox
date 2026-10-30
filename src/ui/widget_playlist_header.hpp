#pragma once
#include <string>
#include <utility>
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/ui.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/tooltip_controller.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

class WidgetPlaylistHeader final : public zincgui::Widget {
  public:
    WidgetPlaylistHeader(const WidgetPlaylistHeader&) = delete;
    WidgetPlaylistHeader(WidgetPlaylistHeader&&) = delete;
    WidgetPlaylistHeader& operator=(const WidgetPlaylistHeader&) = delete;
    WidgetPlaylistHeader& operator=(WidgetPlaylistHeader&&) = delete;

    WidgetPlaylistHeader(zincgui::Root& ui_, size_t collection_id, size_t playlist_id_) : Widget(ui_) {
      const float SCALE = zincbox::ui_scale();

      m_playlist_id = playlist_id_;
      auto playlist = db::playlist_by_id(m_playlist_id);

      layout("ttb m:0 s:0 expand");

      i32 header_height = theme::config().panel_tracklist.header_height;
      i32 header_spacing = theme::config().panel_tracklist.header_spacing;

      auto& header_container = add_child<zincgui::Widget>();
      header_container.height((header_height + 2 * header_spacing) * SCALE).layout("s:0 fill expand");
      header_container.layout().margin.x = 4 * SCALE;
      header_container.layout().margin.y = header_spacing;

      auto& header = header_container.add_child<zincgui::Sprite>("panel_playlists_header");
      header.layout("ltr fill");
      header.layout().spacing = 8 * SCALE;
      header.layout().margin.x = 8 * SCALE;
      header.nine_slice_margin(8.0f).y(6 * SCALE);

      auto& label_author = header.add_child<zincgui::Label>();
      label_author.text(playlist.has_value() ? playlist->get().author_pretty() : "")
        .text_color(theme::config().panel_tracklist.header_author_color)
        .label_anchor(zincgui::Anchor::LEFT)
        .is_drawn(!label_author.text().empty())
        .max_width(label_author.text_extents().x);

      auto& label_name = header.add_child<zincgui::Label>();
      label_name.text(playlist.has_value() ? playlist->get().name : "?")
        .text_color(theme::config().panel_tracklist.header_name_color)
        .label_anchor(zincgui::Anchor::LEFT);

      std::pair<zincgui::Button**, std::string> button_configs[] = {{&m_button_more, "inline_more"},
                                                                    {&m_button_play_next, "inline_play_next"},
                                                                    {&m_button_play, "inline_play"},
                                                                    {&m_button_sort, "inline_sort"}};
      for (auto& [target, name_] : button_configs) {
        *target = &header.add_child<ZincboxButton>(name_);
        (*target)->fixed_width((*target)->width()).fixed_height((*target)->width()).nine_slice_margin(0);
      }

      m_button_play_next->on_press(
        [this, collection_id]() { player::play_playlist(collection_id, m_playlist_id, false); });
      m_button_play->on_press([this, collection_id]() { player::play_playlist(collection_id, m_playlist_id, true); });
    }

    ~WidgetPlaylistHeader() override {}

    void update() override {

      if (m_button_play->is_mouse_hovering()) {
        zincbox::ui::get_tooltip_controller()->show(*m_button_play, tr::get("tooltip.play"),
                                                    zincgui::ToolTipPosition::BELOW, 6);
      } else if (m_button_play_next->is_mouse_hovering()) {
        zincbox::ui::get_tooltip_controller()->show(*m_button_play_next, tr::get("tooltip.play_next"),
                                                    zincgui::ToolTipPosition::BELOW, 6);
      } else if (m_button_sort->is_mouse_hovering()) {
        zincbox::ui::get_tooltip_controller()->show(*m_button_sort, tr::get("tooltip.sort_by"),
                                                    zincgui::ToolTipPosition::BELOW, 6);
      } else if (m_button_more->is_mouse_hovering()) {
        zincbox::ui::get_tooltip_controller()->show(*m_button_more, tr::get("tooltip.more_options"),
                                                    zincgui::ToolTipPosition::BELOW, 6);
      }

      zincgui::Widget::update();
    }

    [[nodiscard]] size_t playlist_id() const noexcept { return m_playlist_id; }
    WidgetPlaylistHeader& playlist_id(size_t id) noexcept {
      m_playlist_id = id;
      return *this;
    }

    [[nodiscard]] zincgui::Button* button_sort() noexcept { return m_button_sort; }
    [[nodiscard]] const zincgui::Button* button_sort() const noexcept { return m_button_sort; }
    [[nodiscard]] zincgui::Button* button_play() noexcept { return m_button_play; }
    [[nodiscard]] const zincgui::Button* button_play() const noexcept { return m_button_play; }
    [[nodiscard]] zincgui::Button* button_play_next() noexcept { return m_button_play_next; }
    [[nodiscard]] const zincgui::Button* button_play_next() const noexcept { return m_button_play_next; }
    [[nodiscard]] zincgui::Button* button_more() noexcept { return m_button_more; }
    [[nodiscard]] const zincgui::Button* button_more() const noexcept { return m_button_more; }

  private:
    size_t m_playlist_id = db::INVALID_ID;
    zincgui::Button* m_button_sort{};
    zincgui::Button* m_button_play{};
    zincgui::Button* m_button_play_next{};
    zincgui::Button* m_button_more{};
};
