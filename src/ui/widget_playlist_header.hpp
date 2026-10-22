#pragma once
#include <string>
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/root.hpp"
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

      playlist_id = playlist_id_;
      auto playlist = db::playlist_by_id(playlist_id);

      set_layout("ttb m:0 s:0 expand");

      i32 header_height = theme::config().panel_tracklist.header_height;
      i32 header_spacing = theme::config().panel_tracklist.header_spacing;

      auto& header_container = add_child<zincgui::Widget>();
      header_container.set_height((header_height + 2 * header_spacing) * SCALE);
      header_container.set_layout("s:0 fill expand");
      header_container.get_layout().margin.x = 4 * SCALE;
      header_container.get_layout().margin.y = header_spacing;

      auto& header = header_container.add_child<zincgui::Sprite>("panel_playlists_header");
      header.set_layout("ltr fill");
      header.get_layout().spacing = 8 * SCALE;
      header.get_layout().margin.x = 8 * SCALE;
      header.set_nine_slice_margin(8.0f);
      header.set_y(6 * SCALE);

      auto& label_author = header.add_child<zincgui::Label>();
      label_author.set_text(playlist.has_value() ? playlist->get().author_pretty() : "");
      label_author.set_text_color(theme::config().panel_tracklist.header_author_color);
      label_author.set_label_anchor(zincgui::Anchor::LEFT);
      label_author.set_is_drawn(!label_author.get_text().empty());
      // label_author.update_text();
      label_author.set_max_width(label_author.get_text_extents().x);
      auto& label_name = header.add_child<zincgui::Label>();
      label_name.set_text(playlist.has_value() ? playlist->get().name : "?");
      label_name.set_text_color(theme::config().panel_tracklist.header_name_color);
      label_name.set_label_anchor(zincgui::Anchor::LEFT);

      std::pair<zincgui::Button**, std::string> button_configs[] = {{&button_more, "inline_more"},
                                                                    {&button_play_next, "inline_play_next"},
                                                                    {&button_play, "inline_play"},
                                                                    {&button_sort, "inline_sort"}};
      for (auto& [target, name_] : button_configs) {
        *target = &header.add_child<ZincboxButton>(name_);
        (*target)->set_min_width((*target)->get_width());
        (*target)->set_max_width((*target)->get_width());
        (*target)->set_nine_slice_margin(0);
      }

      button_play_next->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, false); });
      button_play->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, true); });
    }

    ~WidgetPlaylistHeader() override {}

    void update() override { zincgui::Widget::update(); }

    size_t playlist_id = db::INVALID_ID;
    zincgui::Button* button_sort{};
    zincgui::Button* button_play{};
    zincgui::Button* button_play_next{};
    zincgui::Button* button_more{};
};
