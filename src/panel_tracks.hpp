#pragma once
#include <algorithm>
#include <cmath>
#include <sstream>
#include <glm/common.hpp>
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
static constexpr i32 TRACK_HEIGHT = 22;

class WidgetTrack : public Sprite {
public:
  WidgetTrack(UI& ui_, musicdb::track_id_t track_id_, bool even_) : Sprite(ui_) {
    auto& tracks = musicdb::get_tracks();
    track_id = track_id_;
    even = even_;
    set_texture(even ? "track_bg2" : "track_bg1", false);
    set_nine_slice_margin(4.0);
    set_layout("m:0 s:12 ltr expand");
    set_height(TRACK_HEIGHT);

    std::stringstream track_number;
    track_number << tracks[track_id].track_number;
    label_track_number = &add_child<Label>(track_number.str());
    label_track_number->set_label_anchor(Anchor::LEFT);
    label_track_number->set_size(20, TRACK_HEIGHT);

    label_track_artist = &add_child<Label>(tracks[track_id].artist);
    label_track_artist->set_label_anchor(Anchor::LEFT);
    label_track_artist->set_size(label_track_artist->get_text_extents().x, TRACK_HEIGHT);

    label_track_title = &add_child<Label>(tracks[track_id].title);
    label_track_title->set_label_anchor(Anchor::LEFT);
    label_track_title->set_size(label_track_title->get_text_extents().x, TRACK_HEIGHT);

    label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
    label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
    label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);
  }

  void update() override {
    playing = track_id == player::get_track();

    if (playing != playing_old && playing) {
      set_texture("track_bg_playing", false);
      label_track_number->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.2f);
      label_track_artist->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 0.9f);
      label_track_title->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.5f);
    }
    if (playing != playing_old && !playing) {
      set_texture(even ? "track_bg2" : "track_bg1", false);
      label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
      label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
      label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);
    }

    playing_old = playing;

    Sprite::update();
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    if (is_mouse_hovering() && ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
      if (ev.action == Input::MouseAction::PRESS) {
        pressed = true;
      }
      if (ev.action == Input::MouseAction::RELEASE && pressed) {
        player::play(track_id);
      }
      ev.handled = true;
    }
  }

protected:
  Label* label_track_number{};
  Label* label_track_artist{};
  Label* label_track_title{};
  bool even = false;
  bool playing = false;
  bool playing_old = false;
  musicdb::track_id_t track_id{};
  bool pressed = false;
};

class WidgetAlbum : public Widget {
public:
  WidgetAlbum(UI& ui_, musicdb::album_id_t album_id_) : Widget(ui_) {
    auto& albums = musicdb::get_albums();
    album_id = album_id_;

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
    s.set_text(albums[album_id].title);
    s.set_text_color({1.0, 0.85, 0.95});
    s.set_anchor(Anchor::CENTER);
    s.set_parent_anchor(Anchor::CENTER);
    s.set_label_anchor(Anchor::CENTER);
    s.set_height(ALBUM_HEIGHT);

    bool even = false;
    for (musicdb::track_id_t track_id : albums[album_id].track_ids) {
      add_child<WidgetTrack>(track_id, even);
      even = !even;
    }
  }

  void update() override {
    set_x(12);
    set_width(parent->get_width() - 12);
    Widget::update();
  }

  bool passed_visibility_test = false;
  musicdb::album_id_t album_id;
};

class PanelTracks : public Panel {
public:
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

    recreate();
  }

  void draw() override {
    Panel::draw();
  }

  void recreate() {
    album_scroll_px.clear();
    max_scroll_px = 0;
    for (musicdb::album_id_t album_id : musicdb::get_albums_sorted_by_name()) {
      auto& album = musicdb::get_album(album_id);
      album_scroll_px.emplace_back(max_scroll_px, album_id);
      max_scroll_px += ALBUM_HEIGHT + TRACK_HEIGHT * album.track_ids.size();
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

      create_album(album_scroll_px[i].second, album_start_px);
    }

    for (auto& w : visible_album_widgets) {
      if (!w->passed_visibility_test) {
        w->set_marked_for_deletion(true);
      }
    }

    std::erase_if(visible_album_widgets, [](auto&& w) { return !w->passed_visibility_test; });
  }

  void create_album(musicdb::album_id_t album_id, i32 album_start_px) {
    // album widget already exists
    // FIXME: use hashmap for O(1) lookup
    i32 actual_y = album_start_px - scroll_px;
    for (auto& w : visible_album_widgets) {
      if (w->album_id == album_id) {
        w->set_y(actual_y);
        w->passed_visibility_test = true;

        return;
      }
    }

    // album widget doesn't exist
    auto& w = add_child<WidgetAlbum>(album_id);
    w.set_y(actual_y);
    w.passed_visibility_test = true;
    visible_album_widgets.emplace_back(&w);
  }

  void on_album_clicked(musicdb::album_id_t, size_t album_index_sorted) {
    i32 s = album_scroll_px[album_index_sorted].first;
    target_scroll_px = s;
    scrollbar->set_scroll_offset(s);
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
    scrollbar->set_track_length(height);

    Panel::update();
  }

  void handle_event(Input::InputEventMouseScroll& e) override {
    if (is_mouse_hovering()) {
      scrollbar->scroll(e.offset.y);
      e.handled = true;
    }
  }

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
