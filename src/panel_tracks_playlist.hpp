
#pragma once
#include <vector>
#include "core/musicdb.hpp"
#include "core/player.hpp"
#include "panel_tracks_track.hpp"
#include "theme.hpp"
#include "types.hpp"
#include "ui/panel.hpp"
#include "ui/ui.hpp"

class WidgetAlbum : public Widget {
  public:
    WidgetAlbum(UI& ui_, size_t collection_id, size_t album_id, std::function<void(size_t track, vec2i at)> on_show_playlist_track_actions_popover_) : Widget(ui_) {
      on_show_playlist_track_actions_popover = on_show_playlist_track_actions_popover_;
      std::vector<size_t> collection_ids;
      std::vector<size_t> track_ids;
      auto album = db::playlist_by_id(album_id)->get();
      for (size_t track_id : album.track_ids) {
        collection_ids.emplace_back(collection_id);
        track_ids.emplace_back(track_id);
      }
      create(album_id, album.name, collection_ids, track_ids);
    }

    void create(i32 id, std::u32string title, std::vector<size_t> collection_ids, std::vector<size_t> track_ids) {
      ensure(collection_ids.size() == track_ids.size());
      // ensure(playlist_tracks.size() == track_ids.size() || playlist_tracks.size() == 0);

      album_id = id;

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
      s.set_text(title);
      s.set_text_color({1.0, 0.85, 0.95});
      s.set_anchor(Anchor::CENTER);
      s.set_parent_anchor(Anchor::CENTER);
      s.set_label_anchor(Anchor::CENTER);
      s.set_height(ALBUM_HEIGHT);

      for (size_t i = 0; i < track_ids.size(); i += 1) {
        size_t collection_id = collection_ids[i];
        size_t track_id = track_ids[i];
        auto& w = add_child<WidgetTrack>(collection_ids[i], album_id, track_ids[i], i % 2 == 0);

        w.on_press([track_id, collection_id, this]() {
          player::playing_t play{
            .collection_id = collection_id,
            .playlist_id = album_id,
            .track_id = track_id,
          };
          player::play(play, true);
        });
        // w.on_press_rmb([this, playlist_track, &w]() {
        // if (on_show_playlist_track_actions_popover) {
        // this->on_show_playlist_track_actions_popover(*playlist_track, w.get_position(Anchor::BOTTOM_CENTER));
        // }
        // });
      }
    }

    void update() override {
      set_x(12);
      set_width(parent->get_width() - 12);
      Widget::update();
    }

    bool passed_visibility_test = false;
    size_t album_id;

  protected:
    std::function<void(size_t track, vec2i at)> on_show_playlist_track_actions_popover{};
};
