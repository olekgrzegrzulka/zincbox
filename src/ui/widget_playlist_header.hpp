#pragma once
#include <string>
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class WidgetPlaylistHeader : public Widget {
  public:
    WidgetPlaylistHeader(UI& ui_, size_t collection_id, size_t playlist_id_) : Widget(ui_) {
      playlist_id = playlist_id_;
      auto playlist = db::playlist_by_id(playlist_id);

      set_layout("ttb m:0 s:0 fit expand");

      auto& header_container = add_child<Widget>();
      header_container.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32());
      header_container.set_layout("m:4 s:0 fit fill");

      auto& header = header_container.add_child<Sprite>("panel_playlist_header");
      header.set_anchor(Anchor::CENTER);
      header.set_parent_anchor(Anchor::CENTER);
      header.set_layout("ltr s:8 mx:8 fit fill");
      header.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32() - 20);
      header.set_nine_slice_margin(8.0f);
      header.set_y(6);

      auto& label_author = header.add_child<Label>();
      label_author.set_text(playlist.has_value() ? playlist->get().author : U"?");
      label_author.set_text_color(theme::get_prop("tracklist_header_author_color").as_rgba());
      label_author.set_label_anchor(Anchor::LEFT);
      if (playlist.has_value() && playlist->get().author.empty()) { label_author.set_is_drawn(false); }
      label_author.update();
      label_author.set_max_width(label_author.get_text_extents().x);
      auto& label_name = header.add_child<Label>();
      label_name.set_text(playlist.has_value() ? playlist->get().name : U"?");
      label_name.set_text_color(theme::get_prop("tracklist_header_name_color").as_rgba());
      label_name.set_label_anchor(Anchor::LEFT);

      std::pair<Button**, std::string> button_configs[] = {{&button_more, "inline_more"},
                                                           {&button_play_next, "inline_play_next"},
                                                           {&button_play, "inline_play"},
                                                           {&button_sort, "inline_sort"}};
      for (auto& [target, name] : button_configs) {
        *target = &header.add_child<ZincboxButton>(name);
        (*target)->set_min_width((*target)->get_width());
        (*target)->set_max_width((*target)->get_width());
        (*target)->set_nine_slice_margin(0);
      }

      button_play_next->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, false); });
      button_play->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, true); });
    }

    ~WidgetPlaylistHeader() override {}

    void update() override { Widget::update(); }

    size_t playlist_id = db::INVALID_ID;
    Button* button_sort{};
    Button* button_play{};
    Button* button_play_next{};
    Button* button_more{};
};
