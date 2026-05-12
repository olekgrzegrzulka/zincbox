#include "panel_tracks.hpp"
#include <algorithm>
#include <cmath>
#include "core/musicdb/musicdb.hpp"
#include "panel_tracks_playlist.hpp"
#include "theme.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/ui.hpp"

PanelTracks::PanelTracks(UI& ui_) : Sprite(ui_, "panel_tracks") {
  set_clip_children(true);

  scrollbar = &add_child<ScrollBar>();
  scrollbar->set_anchor(Anchor::LEFT);
  scrollbar->set_parent_anchor(Anchor::LEFT);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) {
    target_scroll_px = scroll_offset;
  });
  scrollbar->set_width(12);
  scrollbar->set_thumb_thickness(12);
  scrollbar->set_track_thickness(12);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);
}

void PanelTracks::draw() {
  Sprite::draw();
}

void PanelTracks::recreate() {
  if (view_type == ViewType::NONE) {
    clear();
  } else if (view_type == ViewType::COLLECTION) {
    recreate(collection_id);
  }
}

void PanelTracks::scroll_to_playlist(size_t target_playlist_id) {
  for (auto [scroll, playlist_id] : album_scroll_px) {
    if (playlist_id == target_playlist_id) {
      scrollbar->set_scroll_offset(scroll);
      return;
    }
  }
}

void PanelTracks::scroll_to_now_playing_album() {
  // Implementation logic here
}

void PanelTracks::update() {
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

  if ((i32)scroll_px != (i32)old_scroll_px || width != old_width) {
    recreate();

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

  Sprite::update();
}

void PanelTracks::handle_event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelTracks::clear() {
  for (auto& w : visible_album_widgets) {
    w->set_marked_for_deletion(true);
  }
  visible_album_widgets.clear();
}

float PanelTracks::get_scroll_px() const {
  return target_scroll_px;
}

void PanelTracks::set_scroll_px(float px) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
}

void PanelTracks::recreate(std::optional<size_t> collection_id_) {
  collection_id = collection_id_;
  if (!collection_id.has_value()) {
    clear();
    return;
  }
  album_scroll_px.clear();
  max_scroll_px = 0;
  auto& c = db::collection_by_id(*collection_id)->get();

  for (size_t album_id : c.playlist_ids) {
    auto& album = db::playlist_by_id(album_id)->get();
    if (album.is_tombstone()) { continue; }
    album_scroll_px.emplace_back(max_scroll_px, album_id);
    max_scroll_px += theme::get_prop("tracklist_playlist_header_height").as_i32() + theme::get_prop("tracklist_track_height").as_i32() * album.get_tracks_count();
  }

  scrollbar->set_content_size(max_scroll_px);

  for (auto& w : visible_album_widgets) {
    w->passed_visibility_test = false;
  }

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
    create_playlist(album_scroll_px[i].second, album_start_px);
  }

  for (auto& w : visible_album_widgets) {
    if (!w->passed_visibility_test) {
      w->set_marked_for_deletion(true);
    }
  }

  std::erase_if(visible_album_widgets, [](auto&& w) { return !w->passed_visibility_test; });
}

void PanelTracks::create_playlist(size_t playlist_id, i32 album_start_px) {
  i32 actual_y = album_start_px - (i32)scroll_px;
  for (auto& w : visible_album_widgets) {
    if (w->playlist_id == playlist_id) {
      w->set_y(actual_y);
      w->passed_visibility_test = true;
      return;
    }
  }

  if (!collection_id.has_value()) { return; }
  auto& w = add_child<WidgetAlbum>(collection_id.value(), playlist_id, on_track_lmb, on_track_rmb);
  w.set_y(actual_y);
  w.passed_visibility_test = true;
  visible_album_widgets.emplace_back(&w);
}
