
#pragma once
#include "core/musicdb.hpp"
#include "panel_tracks_track.hpp"
#include "theme.hpp"
#include "ui/panel.hpp"
#include "ui/ui.hpp"

class WidgetAlbum : public Widget {
  public:
    WidgetAlbum(UI& ui_, size_t collection_id, size_t playlist_id_,
                std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_lmb_,
                std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_rmb_) : Widget(ui_) {
      playlist_id = playlist_id_;
      on_track_lmb = on_track_lmb_;
      on_track_rmb = on_track_rmb_;
      auto playlist = db::playlist_by_id(playlist_id)->get();

      set_layout("ttb m:0 s:0 fit expand");

      auto& album_title_holder = add_child<Widget>();
      album_title_holder.set_height(ALBUM_HEIGHT);
      album_title_holder.set_layout("m:0 s:0 fit fill");

      auto& album_title_panel = album_title_holder.add_child<Panel>(Panel::PanelStyle::Rounded, false);
      album_title_panel.set_anchor(Anchor::CENTER);
      album_title_panel.set_parent_anchor(Anchor::CENTER);
      album_title_panel.set_height(ALBUM_HEIGHT - 20);
      album_title_panel.set_y(6);

      auto& s = album_title_panel.add_child<Label>();
      s.set_text(playlist.name);
      s.set_text_color({1.0, 0.85, 0.95});
      s.set_anchor(Anchor::CENTER);
      s.set_parent_anchor(Anchor::CENTER);
      s.set_label_anchor(Anchor::CENTER);
      s.set_height(ALBUM_HEIGHT);

      bool even = true;
      size_t playlist_track_index = 0;
      for (size_t track_id : playlist.get_track_ids()) {
        even = !even;
        auto w = &add_child<WidgetTrack>(collection_id, playlist_id, track_id, playlist_track_index + 1, even);
        w->on_press([this, collection_id, track_id, playlist_track_index, w]() {
          if (on_track_lmb) {
            this->on_track_lmb(collection_id, playlist_id, track_id, playlist_track_index, w);
          }
        });
        w->on_press_rmb([this, collection_id, track_id, playlist_track_index, w]() {
          if (on_track_rmb) {
            this->on_track_rmb(collection_id, playlist_id, track_id, playlist_track_index, w);
          }
        });

        playlist_track_index += 1;
      }
    }

    void update() override {
      set_x(12);
      set_width(parent->get_width() - 12);
      Widget::update();
    }

    bool passed_visibility_test = false;
    size_t playlist_id;

  protected:
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_rmb{};
};
