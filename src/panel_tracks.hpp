#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <glm/common.hpp>
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "player.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/scrollbar.hpp"
#include "ui/slider.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

static constexpr i32 ALBUM_HEIGHT = 48;
static constexpr i32 TRACK_HEIGHT = 24;

class WidgetTrack : public Button {
  public:
    WidgetTrack(UI& ui_, const musicdb::Track* track, bool even_, std::optional<musicdb::playlist_track> playlist_track_ = std::nullopt) : Button(ui_) {
      playlist_track = playlist_track_;
      track_id = track->track_id;
      album_id = track->album_id;
      collection_id = track->collection_id;
      even = even_;
      std::string txt = even ? "track_bg2" : "track_bg1";
      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      set_texture(txt, false);
      set_nine_slice_margin(4.0);
      set_layout("m:8 s:8 ltr expand");
      set_height(TRACK_HEIGHT);

      std::stringstream track_number;
      track_number << track->track_number;
      label_track_number = &add_child<Label>(track_number.str());
      label_track_number->set_label_anchor(Anchor::LEFT);
      label_track_number->set_size(20, TRACK_HEIGHT);
      label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
      label_track_number->set_x(5);

      label_track_artist = &add_child<Label>(track->artist);
      label_track_artist->set_label_anchor(Anchor::LEFT);
      label_track_artist->set_height(TRACK_HEIGHT);
      label_track_artist->set_x(label_track_number->get_x() + label_track_number->get_width() + 5);
      label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);

      label_track_title = &add_child<Label>(track->title);
      label_track_title->set_x(label_track_artist->get_x() + label_track_artist->get_width() + 5);
      label_track_title->set_label_anchor(Anchor::LEFT);
      label_track_title->set_height(TRACK_HEIGHT);
      label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);

      panel_right_side = &add_child<Panel>();
      panel_right_side->set_height(TRACK_HEIGHT);
      panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
      panel_right_side->set_layout("m:8 s:8 rtl expand fit");
      panel_right_side->set_anchor(Anchor::RIGHT);
      panel_right_side->set_parent_anchor(Anchor::RIGHT);
      panel_right_side->set_ignore_parents_layout(true);

      i32 length_s = track->length_seconds;
      i32 length_m = length_s / 60;
      length_s %= 60;
      std::stringstream ss;
      ss << std::right << std::setfill('0') << std::setw(0) << length_m << ":" << std::setw(2) << length_s;
      label_track_length = &panel_right_side->add_child<Label>(ss.str());
      label_track_length->set_height(TRACK_HEIGHT);
      label_track_length->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
      label_track_length->set_resize_to_text_extents(false);
    }

    void update() override {
      playing = (player::get_playing() &&
                 track_id == player::get_playing()->track_id &&
                 collection_id == player::get_playing()->collection_id);

      if (playing != playing_old && playing) {
        panel_right_side->set_texture("track_bg_playing", false);
        std::string txt = "track_bg_playing";
        set_texture_idle(txt);
        set_texture_hovered(txt);
        set_texture_disabled(txt);
        set_texture_pressed(txt);
        set_texture(txt, false);
        label_track_number->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.2f);
        label_track_artist->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 0.9f);
        label_track_title->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.5f);
      }
      if (playing != playing_old && !playing) {
        panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
        std::string txt = even ? "track_bg2" : "track_bg1";
        set_texture_idle(txt);
        set_texture_hovered(txt);
        set_texture_disabled(txt);
        set_texture_pressed(txt);
        set_texture(txt, false);
        label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
        label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
        label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);
      }

      playing_old = playing;

      Sprite::update();
    }

    // void handle_event(Input::InputEventMouseButton& ev) override {
    //   if (is_mouse_hovering() && ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
    //     if (ev.action == Input::MouseAction::PRESS) {
    //       pressed = true;
    //     }
    //     if (ev.action == Input::MouseAction::RELEASE && pressed) {
    //       player::now_playing_t now_playing{
    //         .track_id = track_id,
    //         .album_id = album_id,
    //         .collection_id = collection_id,
    //         .playlist_track = playlist_track,
    //       };
    //       player::play(now_playing, track_id);
    //     }
    //     ev.handled = true;
    //   }
    // }

  protected:
    Label* label_track_number{};
    Label* label_track_artist{};
    Label* label_track_title{};
    Panel* panel_right_side{};
    Label* label_track_length{};
    bool even = false;
    bool playing = false;
    bool playing_old = false;
    musicdb::collection_id_t collection_id{};
    musicdb::album_id_t album_id{};
    musicdb::track_id_t track_id{};
    std::optional<musicdb::playlist_track> playlist_track{};
    bool pressed = false;
};

class WidgetAlbum : public Widget {
  public:
    WidgetAlbum(UI& ui_, const musicdb::Album* album, std::function<void(musicdb::playlist_track track, vec2i at)> on_show_playlist_track_actions_popover_) : Widget(ui_) {
      on_show_playlist_track_actions_popover = on_show_playlist_track_actions_popover_;
      std::vector<musicdb::collection_id_t> collection_ids;
      std::vector<musicdb::track_id_t> track_ids;
      for (musicdb::track_id_t track_id : album->track_ids) {
        collection_ids.emplace_back(album->collection_id);
        track_ids.emplace_back(track_id);
      }
      create(album->album_id, album->title, collection_ids, track_ids);
    }

    WidgetAlbum(UI& ui_, const musicdb::Playlist* playlist, std::function<void(musicdb::playlist_track track, vec2i at)> on_show_playlist_track_actions_popover_) : Widget(ui_) {
      on_show_playlist_track_actions_popover = on_show_playlist_track_actions_popover_;
      std::vector<musicdb::collection_id_t> collection_ids;
      std::vector<musicdb::track_id_t> track_ids;
      std::vector<musicdb::playlist_track> playlist_tracks;
      for (auto playlist_track : playlist->get_tracks()) {
        collection_ids.emplace_back(playlist_track.collection_id);
        track_ids.emplace_back(playlist_track.track_id);
        playlist_tracks.emplace_back(playlist_track);
      }

      create(playlist->get_id(), std::u32string{playlist->get_name()}, collection_ids, track_ids, playlist_tracks);
    }

    void create(i32 id, std::u32string title, std::vector<musicdb::collection_id_t> collection_ids, std::vector<musicdb::track_id_t> track_ids, std::vector<musicdb::playlist_track> playlist_tracks = {}) {
      ensure(collection_ids.size() == track_ids.size());
      ensure(playlist_tracks.size() == track_ids.size() || playlist_tracks.size() == 0);

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
        auto* track = musicdb::get_track(collection_ids[i], track_ids[i]);
        auto playlist_track = ((playlist_tracks.size() > 0) ? std::make_optional(playlist_tracks[i]) : std::nullopt);
        auto& w = add_child<WidgetTrack>(track, i % 2 == 0, playlist_track);

        if (playlist_track.has_value()) {
          w.on_press([playlist_track]() {
            player::now_playing_t now_playing{
              .track_id = playlist_track->track_id,
              .album_id = playlist_track->album_id,
              .collection_id = playlist_track->collection_id,
              .playlist_track = playlist_track,
            };
            player::play(now_playing, true);
          });
          w.on_press_rmb([this, playlist_track, &w]() {
            if (on_show_playlist_track_actions_popover) {
              this->on_show_playlist_track_actions_popover(*playlist_track, w.get_position(Anchor::BOTTOM_CENTER));
            }
          });
        } else {
          w.on_press([track]() {
            player::now_playing_t now_playing{
              .track_id = track->track_id,
              .album_id = track->album_id,
              .collection_id = track->collection_id,
              .playlist_track = std::nullopt,
            };
            player::play(now_playing, track->track_id);
          });
        }
      }
    }

    void update() override {
      set_x(12);
      set_width(parent->get_width() - 12);
      Widget::update();
    }

    bool passed_visibility_test = false;
    musicdb::album_id_t album_id;

  protected:
    std::function<void(musicdb::playlist_track track, vec2i at)> on_show_playlist_track_actions_popover{};
};

class PanelTracks : public Panel {
  public:
    enum class ViewType {
      NONE,
      COLLECTION,
      PLAYLISTS,
    };

    PanelTracks(UI& ui_) : Panel(ui_, Panel::PanelStyle::RectangularDark, false) {
      set_clip_children(true);

      scrollbar = &add_child<ScrollBar>();
      scrollbar->set_anchor(Anchor::LEFT);
      scrollbar->set_parent_anchor(Anchor::LEFT);
      scrollbar->on_value_changed([&](i32 old, i32 scroll_offset) {
        target_scroll_px = scroll_offset;
      });
      scrollbar->set_width(12);
      scrollbar->set_thumb_thickness(12);
      scrollbar->set_track_thickness(12);
      scrollbar->set_orientation(SliderOrientation::VERTICAL);
    }

    void draw() override {
      Panel::draw();
    }

    void recreate() {
      if (view_type == ViewType::NONE) {
        clear();
      } else if (view_type == ViewType::COLLECTION) {
        recreate(collection_id);
      } else if (view_type == ViewType::PLAYLISTS) {
        recreate_playlist();
      }
    }

    void scroll_to_playlist(musicdb::playlist_id_t i) {
      i32 s = album_scroll_px[i].first;
      target_scroll_px = s;
      scrollbar->set_scroll_offset(s);
    }

    void scroll_to_album(size_t album_index_sorted) {
      i32 s = album_scroll_px[album_index_sorted].first;
      target_scroll_px = s;
      scrollbar->set_scroll_offset(s);
    }

    void scroll_to_now_playing_album() {
      auto now_playing = player::get_playing();

      if (!collection_id.has_value()) { return; }
      if (!now_playing.has_value()) { return; }
      if (now_playing->collection_id != *collection_id) { return; }

      musicdb::album_id_t album_id = now_playing->album_id;
      auto* collection = musicdb::get_collection(*collection_id);
      auto& sorted_indices = collection->get_albums_sorted_by_name();

      size_t i = 0;
      for (size_t album_id_ : sorted_indices) {
        if (album_id_ == album_id) {
          scroll_to_album(i);
          return;
        }
        i += 1;
      }
    }

    void update() override {
      scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

      if ((i32)scroll_px != (i32)old_scroll_px || width != old_width) {
        recreate();

        // FIXME: why?
        for (auto& v : visible_album_widgets) {
          ui.mark_dirty_recursive(v);
        }
      }

      old_scroll_px = scroll_px;
      old_width = width;

      double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
      scroll_px = std::lerp(scroll_px, target_scroll_px, t);

      scrollbar->set_page_size(height);
      scrollbar->set_height(height);

      Panel::update();
    }

    void handle_event(Input::InputEventMouseScroll& e) override {
      if (is_mouse_hovering()) {
        scrollbar->scroll(e.offset.y);
        e.handled = true;
      }
    }

    void clear() {
      for (auto& w : visible_album_widgets) {
        w->set_marked_for_deletion(true);
      }
      visible_album_widgets.clear();
    }

  protected:
    void recreate(std::optional<musicdb::collection_id_t> collection_id_) {
      collection_id = collection_id_;
      if (!collection_id.has_value()) {
        clear();
        return;
      }
      album_scroll_px.clear();
      max_scroll_px = 0;
      auto* c = musicdb::get_collection(*collection_id);
      for (musicdb::album_id_t album_id : c->get_albums_sorted_by_name()) {
        auto* album = c->get_album(album_id);
        album_scroll_px.emplace_back(max_scroll_px, album_id);
        max_scroll_px += ALBUM_HEIGHT + TRACK_HEIGHT * album->track_ids.size();
      }

      scrollbar->set_content_size(max_scroll_px);

      for (auto& w : visible_album_widgets) {
        w->passed_visibility_test = false;
      }

      // FIXME: binary search perhaps
      for (i32 i = 0; i < (i32)album_scroll_px.size(); i += 1) {
        i32 album_start_px = album_scroll_px[i].first;
        i32 album_end_px = max_scroll_px;
        if (i + 1 < (i32)album_scroll_px.size()) {
          album_end_px = album_scroll_px[i + 1].first;
        }

        i32 view_start_px = scroll_px;
        i32 view_end_px = scroll_px + get_height();

        if (album_end_px < view_start_px) { continue; }
        if (album_start_px > view_end_px) { break; }
        auto* album = c->get_album(album_scroll_px[i].second);
        create_album(album, album_start_px);
      }

      for (auto& w : visible_album_widgets) {
        if (!w->passed_visibility_test) {
          w->set_marked_for_deletion(true);
        }
      }

      std::erase_if(visible_album_widgets, [](auto&& w) { return !w->passed_visibility_test; });
    }

    void recreate_playlist() {
      album_scroll_px.clear();
      max_scroll_px = 0;
      for (auto& playlist : musicdb::get_playlists()) {
        album_scroll_px.emplace_back(max_scroll_px, playlist.get_id());
        max_scroll_px += ALBUM_HEIGHT + TRACK_HEIGHT * playlist.get_tracks_count();
      }

      scrollbar->set_content_size(max_scroll_px);

      for (auto& w : visible_album_widgets) {
        w->passed_visibility_test = false;
      }

      // FIXME: binary search perhaps
      for (i32 i = 0; i < (i32)album_scroll_px.size(); i += 1) {
        i32 album_start_px = album_scroll_px[i].first;
        i32 album_end_px = max_scroll_px;
        if (i + 1 < (i32)album_scroll_px.size()) {
          album_end_px = album_scroll_px[i + 1].first;
        }

        i32 view_start_px = scroll_px;
        i32 view_end_px = scroll_px + get_height();

        if (album_end_px < view_start_px) { continue; }
        if (album_start_px > view_end_px) { break; }
        create_playlist(&musicdb::get_playlists()[i], album_start_px);
      }

      for (auto& w : visible_album_widgets) {
        if (!w->passed_visibility_test) {
          w->set_marked_for_deletion(true);
        }
      }

      std::erase_if(visible_album_widgets, [](auto&& w) { return !w->passed_visibility_test; });
    }

    void create_album(const musicdb::Album* album, i32 album_start_px) {
      // album widget already exists
      // FIXME: use hashmap for O(1) lookup
      i32 actual_y = album_start_px - scroll_px;
      for (auto& w : visible_album_widgets) {
        if (w->album_id == album->album_id) {
          w->set_y(actual_y);
          w->passed_visibility_test = true;

          return;
        }
      }

      // album widget doesn't exist
      auto& w = add_child<WidgetAlbum>(album, nullptr);
      w.set_y(actual_y);
      w.passed_visibility_test = true;
      visible_album_widgets.emplace_back(&w);
      debug_warn("SET");
    }

    void create_playlist(const musicdb::Playlist* playlist, i32 album_start_px) {
      // album widget already exists
      // FIXME: use hashmap for O(1) lookup
      i32 actual_y = album_start_px - scroll_px;
      for (auto& w : visible_album_widgets) {
        if (w->album_id == playlist->get_id()) {
          w->set_y(actual_y);
          w->passed_visibility_test = true;
          return;
        }
      }

      // album widget doesn't exist
      std::function<void(musicdb::playlist_track track, vec2i at)> fun = [this](musicdb::playlist_track t, vec2i at) {
        if (this->on_show_playlist_track_actions_popover) {
          this->on_show_playlist_track_actions_popover(t, at);
        }
      };
      auto& w = add_child<WidgetAlbum>(playlist, fun);
      w.set_y(actual_y);
      w.passed_visibility_test = true;
      visible_album_widgets.emplace_back(&w);
    }

  public:
    ViewType view_type = ViewType::NONE;
    std::optional<musicdb::collection_id_t> collection_id{};
    std::function<void(musicdb::playlist_track track, vec2i at)> on_show_playlist_track_actions_popover{};

  protected:
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    i32 old_width{};
    i32 max_scroll_px{};
    ScrollBar* scrollbar{};
    std::vector<std::pair<i32, musicdb::album_id_t>> album_scroll_px;
    std::vector<WidgetAlbum*> visible_album_widgets;
};
