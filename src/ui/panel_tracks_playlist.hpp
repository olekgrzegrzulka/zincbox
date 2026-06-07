#pragma once
#include "core/musicdb/musicdb.hpp"
#include "core/player.hpp"
#include "panel_tracks_track.hpp"
#include "theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/tooltip.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class WidgetAlbum : public Widget {
  protected:
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack* widget)>
      on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack* widget)>
      on_track_rmb{};
    std::vector<WidgetTrack*> track_widgets;
    Signal<>::slot_key slot_on_track_changed;

  public:
    WidgetAlbum(UI& ui_, size_t collection_id, size_t playlist_id_, decltype(on_track_lmb) on_track_lmb_,
                decltype(on_track_rmb) on_track_rmb_)
      : Widget(ui_) {
      playlist_id = playlist_id_;
      on_track_lmb = on_track_lmb_;
      on_track_rmb = on_track_rmb_;
      auto playlist = db::playlist_by_id(playlist_id)->get();

      set_layout("ttb m:0 s:0 fit expand");

      auto& header_container = add_child<Widget>();
      header_container.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32());
      header_container.set_layout("m:4 s:0 fit fill");

      auto& header = header_container.add_child<Sprite>("panel_playlist_header");
      header.set_anchor(Anchor::CENTER);
      header.set_parent_anchor(Anchor::CENTER);
      // header.set_layout("ltr m:8 s:8");
      header.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32() - 20);
      header.set_nine_slice_margin(8.0f);
      header.set_y(6);

      auto& labels = header.add_child<Widget>();
      labels.set_anchor(Anchor::LEFT);
      labels.set_parent_anchor(Anchor::LEFT);

      auto& label_author = labels.add_child<Label>();
      label_author.set_text(playlist.author);
      // hack to get text extents to update :(
      label_author.mark_dirty();
      label_author.update();
      label_author.set_text_color(theme::get_prop("tracklist_header_author_color").as_rgba());
      label_author.set_anchor(Anchor::LEFT);
      label_author.set_parent_anchor(Anchor::LEFT);
      label_author.set_label_anchor(Anchor::LEFT);
      label_author.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32());
      label_author.set_x(8);
      if (playlist.author.empty()) { label_author.set_is_drawn(false); }

      auto& label_name = labels.add_child<Label>();
      label_name.set_text(playlist.name);
      label_name.set_text_color(theme::get_prop("tracklist_header_name_color").as_rgba());
      label_name.set_anchor(Anchor::LEFT);
      label_name.set_parent_anchor(Anchor::LEFT);
      label_name.set_label_anchor(Anchor::LEFT);
      label_name.set_height(theme::get_prop("tracklist_playlist_header_height").as_i32());
      label_name.set_x(label_author.get_is_drawn() ? label_author.get_width() + 14 : 8);

      auto& buttons = header.add_child<Widget>();
      buttons.set_anchor(Anchor::RIGHT);
      buttons.set_parent_anchor(Anchor::RIGHT);
      buttons.set_layout("rtl m:8 s:6");
      button_play_next = &buttons.add_child<ZincboxButton>("inline_play_next");
      button_play = &buttons.add_child<ZincboxButton>("inline_play");
      button_play_next->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, false); });
      button_play->on_press([this, collection_id]() { player::play_playlist(collection_id, playlist_id, true); });

      bool even = true;
      size_t playlist_track_index = 0;
      for (size_t track_id : playlist.get_track_ids()) {
        even = !even;
        auto w = &add_child<WidgetTrack>(track_id, playlist_track_index + 1, even);
        track_widgets.emplace_back(w);
        w->on_press([this, collection_id, track_id, playlist_track_index, w]() {
          if (on_track_lmb) { this->on_track_lmb(collection_id, playlist_id, track_id, playlist_track_index, w); }
        });
        w->on_press_rmb([this, collection_id, track_id, playlist_track_index, w]() {
          if (on_track_rmb) { this->on_track_rmb(collection_id, playlist_id, track_id, playlist_track_index, w); }
        });

        if (auto playing = player::get_playing()) {
          w->set_highlighted(w->track_id == playing->track_id && playlist_id == playing->playlist_id &&
                             collection_id == playing->collection_id);
        }

        playlist_track_index += 1;
      }

      slot_on_track_changed = player::signal_on_track_changed.connect([this, collection_id]() {
        for (auto* w : track_widgets) {
          w->set_highlighted(player::get_playing().has_value() && w->track_id == player::get_playing()->track_id &&
                             playlist_id == player::get_playing()->playlist_id &&
                             collection_id == player::get_playing()->collection_id);
        }
      });
    }

    ~WidgetAlbum() override { player::signal_on_track_changed.disconnect(slot_on_track_changed); }

    void update() override {
      set_x(12);
      set_width(parent->get_width() - 12);
      Widget::update();
    }

    bool passed_visibility_test = false;
    size_t playlist_id;
    Button* button_play{};
    Button* button_play_next{};
};
