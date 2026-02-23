#pragma once
#include <algorithm>
#include <cmath>
#include <sstream>
#include <glm/common.hpp>
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/scrollbar.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

static constexpr i32 ALBUM_HEIGHT = 28;
static constexpr i32 TRACK_HEIGHT = 20;

static constexpr double SCROLL_SPEED = 12.0;
static constexpr double SCROLL_MAX_SPEED = 100.0;
static constexpr double SCROLL_FRICTION_LINEAR = 0.17;
static constexpr double SCROLL_FRICTION_QUADRATIC = 0.005;

class WidgetAlbum : public Widget {
public:
  WidgetAlbum(UI& ui_, const musicdb::Album* album_) : Widget(ui_), album(album_) {
    set_layout("ttb m:0 s:0 fit expand");

    auto& album_title_panel = add_child<Panel>(Panel::PanelStyle::Rounded, false);
    album_title_panel.set_height(ALBUM_HEIGHT);

    auto& s = album_title_panel.add_child<Label>();
    s.set_text(album->title);
    s.set_text_color({1.0, 0.85, 0.95});
    s.set_anchor(Anchor::CENTER);
    s.set_parent_anchor(Anchor::CENTER);
    s.set_label_anchor(Anchor::CENTER);
    s.set_height(ALBUM_HEIGHT);

    for (auto& track : album->tracks) {
      auto& labels = add_child<Widget>();
      labels.set_layout("m:0 s:12 ltr expand");
      labels.set_height(TRACK_HEIGHT);

      std::stringstream track_number;
      track_number << track.track;
      auto& label_track_number = labels.add_child<Label>(track_number.str());
      label_track_number.set_label_anchor(Anchor::LEFT);
      label_track_number.set_size(20, TRACK_HEIGHT);
      label_track_number.set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);

      auto& label_track_artist = labels.add_child<Label>(track.artist);
      label_track_artist.set_label_anchor(Anchor::LEFT);
      label_track_artist.set_size(label_track_artist.get_text_extents().x, TRACK_HEIGHT);
      label_track_artist.set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);

      auto& label_track_title = labels.add_child<Label>(track.title);
      label_track_title.set_label_anchor(Anchor::LEFT);
      label_track_title.set_size(label_track_title.get_text_extents().x, TRACK_HEIGHT);
      label_track_title.set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);
    }
  }

  void update() override {
    set_x(12);
    set_width(parent->get_width() - 12);
    Widget::update();
  }

  bool passed_visibility_test = false;
  const musicdb::Album* album;
};

class PanelTracks : public Panel {
public:
  PanelTracks(UI& ui_) : Panel(ui_, Panel::PanelStyle::RoundedDark, false) {
    set_clip_children(true);

    scrollbar = &add_child<ScrollBar>();
    scrollbar->set_anchor(Anchor::LEFT);
    scrollbar->set_parent_anchor(Anchor::LEFT);
    scrollbar->on_scroll_offset_changed([&](i32 scroll_offset) {
      target_scroll_px = scroll_offset;
    });

    recreate();
  }

  void draw() override {
    Panel::draw();
  }

  void recreate() {
    album_scroll_px.clear();
    max_scroll_px = 0;
    for (auto& album : musicdb::get_albums()) {
      album_scroll_px.emplace_back(max_scroll_px, &album);
      max_scroll_px += ALBUM_HEIGHT + TRACK_HEIGHT * album.tracks.size();
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

  void create_album(const musicdb::Album* album, i32 album_start_px) {
    // album widget already exists
    // FIXME: use hashmap for O(1) lookup
    i32 actual_y = album_start_px - scroll_px;
    for (auto& w : visible_album_widgets) {
      if (w->album == album) {
        w->set_y(actual_y);
        w->passed_visibility_test = true;

        return;
      }
    }

    // album widget doesn't exist
    auto& w = add_child<WidgetAlbum>(album);
    w.set_y(actual_y);
    w.passed_visibility_test = true;
    visible_album_widgets.emplace_back(&w);
  }

  void on_album_clicked(i32 id) {
    i32 s = album_scroll_px[id].first;
    target_scroll_px = s;
    scrollbar->set_scroll_offset(s);
  }

  void update() override {
    scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

    if ((i32)scroll_px != (i32)old_scroll_px || width != old_width) {
      recreate();

      // FIXME: WHYYYYYYYYYYYY ????????????????
      for (auto& v : visible_album_widgets) {
        ui.mark_dirty_recursive(v);
      }
    }

    old_scroll_px = scroll_px;
    old_width = width;

    double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.3, 0.75);
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

protected:
  double scroll_px{};
  double target_scroll_px{};
  double old_scroll_px{};
  i32 old_width{};
  i32 max_scroll_px{};
  ScrollBar* scrollbar{};
  std::vector<std::pair<i32, const musicdb::Album*>> album_scroll_px;
  std::vector<WidgetAlbum*> visible_album_widgets;
};
