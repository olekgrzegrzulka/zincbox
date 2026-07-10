#pragma once
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
      header.set_layout("rtl s:8 fit fill");
      header.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32() - 20);
      header.set_nine_slice_margin(8.0f);
      header.set_y(6);

      auto& buttons = header.add_child<Widget>();
      buttons.set_layout("rtl m:8 s:6 fit");
      button_more = &buttons.add_child<ZincboxButton>("inline_more");
      button_play_next = &buttons.add_child<ZincboxButton>("inline_play_next");
      button_play = &buttons.add_child<ZincboxButton>("inline_play");
      button_sort = &buttons.add_child<ZincboxButton>("inline_sort");
      button_more->set_nine_slice_margin(0);
      button_play_next->set_nine_slice_margin(0);
      button_play->set_nine_slice_margin(0);
      button_sort->set_nine_slice_margin(0);
      buttons.update();
      buttons.set_max_width(buttons.get_width());
      button_play_next->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, false); });
      button_play->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, true); });

      auto& labels = header.add_child<Widget>();
      labels.set_layout("ltr m:8 s:6");
      labels.set_clip_children(true);
      auto& label_author = labels.add_child<Label>();
      label_author.set_text(playlist.has_value() ? playlist->get().author : U"?");
      label_author.set_text_color(theme::get_prop("tracklist_header_author_color").as_rgba());
      if (playlist.has_value() && playlist->get().author.empty()) { label_author.set_is_drawn(false); }
      auto& label_name = labels.add_child<Label>();
      label_name.set_text(playlist.has_value() ? playlist->get().name : U"?");
      label_name.set_text_color(theme::get_prop("tracklist_header_name_color").as_rgba());

      label_author.update();
      label_name.update();
    }

    ~WidgetPlaylistHeader() override {}

    void update() override { Widget::update(); }

    size_t playlist_id = db::INVALID_ID;
    Button* button_sort{};
    Button* button_play{};
    Button* button_play_next{};
    Button* button_more{};
};
