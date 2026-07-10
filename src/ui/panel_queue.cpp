#include "panel_queue.hpp"
#include <algorithm>
#include <cmath>
#include "core/player.hpp"
#include "widget_track.hpp"
#include "theme.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/ui.hpp"

PanelQueue::PanelQueue(UI& ui_) : Sprite(ui_, "panel_queue") {
  set_clip_children(true);

  scrollbar = &add_child<ScrollBar>();
  scrollbar->set_anchor(Anchor::LEFT);
  scrollbar->set_parent_anchor(Anchor::LEFT);
  scrollbar->on_value_changed([&, this](i32, i32 scroll_offset) { target_scroll_px = scroll_offset; });
  scrollbar->set_width(12);
  scrollbar->set_thumb_thickness(12);
  scrollbar->set_track_thickness(12);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);

  slot_on_queue_changed = player::signal_on_queue_changed.connect([this](bool track_appended_to_back) {
    if (track_appended_to_back) {
      this->on_queue_appended_to_back();
    } else {
      this->on_queue_changed();
    }
  });
  slot_on_track_changed = player::signal_on_track_changed.connect([this]() {
    for (size_t queue_i = 0; queue_i < queue_tracks.size(); queue_i += 1) {
      auto playing_index = player::get_playing_index();
      queue_tracks[queue_i]->set_highlighted(playing_index.has_value() && queue_i == playing_index.value());
    }
  });
}

PanelQueue::~PanelQueue() {
  player::signal_on_queue_changed.disconnect(slot_on_queue_changed);
  player::signal_on_track_changed.disconnect(slot_on_track_changed);
}

void PanelQueue::draw() { Sprite::draw(); }

void PanelQueue::on_view_changed() {
  i32 i = -1;
  for (auto* track : queue_tracks) {
    i += 1;

    track->set_x(scrollbar->get_width());
    track->set_y(i * theme::get_prop("tracklist_track_height").as_i32() - scroll_px);
    track->set_width(width - scrollbar->get_width());
    track->set_height(theme::get_prop("tracklist_track_height").as_i32());
  }
}

void PanelQueue::on_queue_appended_to_back() {
  size_t queue_size = player::get_playing_queue().size();
  if (queue_size == 0) { return; }
  i32 queue_i = queue_size - 1;
  player::playing_t p = player::get_playing_queue()[queue_i];
  auto* track = &add_child<WidgetTrack>();
  track->track_id(p.track_id).track_number(queue_i + 1);
  queue_tracks.emplace_back(track);

  track->on_press([this, queue_i, track]() {
    if (this->on_queue_element_lmb) { this->on_queue_element_lmb(queue_i, track); }
  });
  track->on_press_rmb([this, queue_i, track]() {
    if (this->on_queue_element_rmb) { this->on_queue_element_rmb(queue_i, track); }
  });

  max_scroll_px = queue_size * theme::get_prop("tracklist_track_height").as_i32();
  scrollbar->set_content_size(max_scroll_px);

  on_view_changed();
}

void PanelQueue::on_queue_changed_at(size_t queue_i) {
  if (player::get_playing_queue().size() <= queue_i) { return; }
  if (queue_tracks.size() <= queue_i) { return; }
  auto p = player::get_playing_queue()[queue_i];
  queue_tracks[queue_i]->track_id(p.track_id);
}

void PanelQueue::on_queue_changed() {
  clear();

  i32 queue_i = -1;
  for (player::playing_t p : player::get_playing_queue()) {
    queue_i += 1;

    auto* track = &add_child<WidgetTrack>();
    track->track_id(p.track_id).track_number(queue_i + 1);
    queue_tracks.emplace_back(track);

    track->on_press([this, queue_i, track]() {
      if (this->on_queue_element_lmb) { this->on_queue_element_lmb(queue_i, track); }
    });
    track->on_press_rmb([this, queue_i, track]() {
      if (this->on_queue_element_rmb) { this->on_queue_element_rmb(queue_i, track); }
    });

    auto playing_index = player::get_playing_index();
    queue_tracks[queue_i]->set_highlighted(playing_index.has_value() && (size_t)queue_i == playing_index.value());

    max_scroll_px = player::get_playing_queue().size() * theme::get_prop("tracklist_track_height").as_i32();
    scrollbar->set_content_size(max_scroll_px);
  }

  on_view_changed();
}

void PanelQueue::update() {
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

  if ((i32)scroll_px != (i32)old_scroll_px || width != old_width) { on_view_changed(); }

  old_scroll_px = scroll_px;
  old_width = width;

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  scrollbar->set_page_size(height);
  scrollbar->set_height(height);

  Sprite::update();
}

void PanelQueue::event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelQueue::clear() {
  for (auto* w : queue_tracks) {
    w->set_marked_for_deletion(true);
  }
  queue_tracks.clear();
}
