#pragma once
#include <algorithm>
#include <sstream>
#include <glm/common.hpp>
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
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
    s.set_anchor(Anchor::CENTER);
    s.set_parent_anchor(Anchor::CENTER);
    s.set_label_anchor(Anchor::CENTER);
    s.set_height(ALBUM_HEIGHT);

    for (auto& track : album->tracks) {
      auto& t = add_child<Label>();
      std::wstringstream track_number;
      track_number << std::left << std::setw(4) << track.track;
      t.set_text(track_number.str() + track.title);
      t.set_label_anchor(Anchor::LEFT);
      t.set_height(TRACK_HEIGHT);
      t.set_text_color({0.75, 0.75, 0.75});
    }
  }

  void update() override {
    set_width(parent->get_width());
    Widget::update();
  }

  bool passed_visibility_test = false;
  const musicdb::Album* album;
};

class PanelTracks : public Panel {
public:
  PanelTracks(UI& ui_) : Panel(ui_, Panel::PanelStyle::RoundedDark, false) {
    set_clip_children(true);
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

  void update() override {
    double old_scroll_px = scroll_px;

    scroll_px += scroll_velocity;

    double apply_friction = scroll_velocity * scroll_velocity * SCROLL_FRICTION_QUADRATIC;
    apply_friction += std::abs(scroll_velocity) * SCROLL_FRICTION_LINEAR;
    apply_friction = std::min(apply_friction, std::abs(scroll_velocity));

    scroll_velocity -= apply_friction * glm::sign(scroll_velocity);
    scroll_velocity = std::clamp(scroll_velocity, -SCROLL_MAX_SPEED, SCROLL_MAX_SPEED);
    scroll_px = std::clamp(scroll_px, 0.0, (double)(max_scroll_px - get_height()));

    if ((i32)scroll_px != (i32)old_scroll_px) {
      recreate();

      // FIXME: WHYYYYYYYYYYYY ????????????????
      for (auto& v : visible_album_widgets) {
        for (auto& c : v->get_children()) {
          c->mark_dirty();
        }
      }
    }

    Panel::update();
  }

  void
  handle_event(Input::InputEventMouseScroll& e) override {
    if (is_mouse_hovering()) {
      e.handled = true;
      scroll_velocity -= e.offset.y * SCROLL_SPEED;
    }
  }

protected:
  double scroll_px{};
  double scroll_velocity{};
  i32 max_scroll_px{};
  std::vector<std::pair<i32, const musicdb::Album*>> album_scroll_px;
  std::vector<WidgetAlbum*> visible_album_widgets;
};
