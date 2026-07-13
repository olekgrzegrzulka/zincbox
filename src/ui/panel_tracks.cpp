#include "panel_tracks.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <complex.h>
#include "common/input.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "theme.hpp"
#include "tr.hpp"
#include "ui/widget_track.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"
#include "widget_playlist_header.hpp"

PanelTracks::PanelTracks(UI& ui_) : Sprite(ui_, "panel_tracks") {
  set_clip_children(true);

  scrollbar = &add_child<ScrollBar>();
  scrollbar->set_anchor(Anchor::LEFT);
  scrollbar->set_parent_anchor(Anchor::LEFT);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });
  scrollbar->set_width(12);
  scrollbar->set_thumb_thickness(12);
  scrollbar->set_track_thickness(12);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);

  button_play_tooltip = &add_child<ToolTip>(tr::get("tooltip.play"), ToolTipPosition::BELOW, 8);
  button_play_next_tooltip = &add_child<ToolTip>(tr::get("tooltip.play_next"), ToolTipPosition::BELOW, 8);
  button_sort_tooltip = &add_child<ToolTip>(tr::get("tooltip.sort_by"), ToolTipPosition::BELOW, 8);
  button_more_tooltip = &add_child<ToolTip>(tr::get("tooltip.more_options"), ToolTipPosition::BELOW, 8);
  button_play_tooltip->set_is_drawn(false);
  button_play_next_tooltip->set_is_drawn(false);
  button_sort_tooltip->set_is_drawn(false);
  button_more_tooltip->set_is_drawn(false);
  button_play_tooltip->set_parent_anchor(Anchor::TOP_LEFT);
  button_play_tooltip->set_anchor(Anchor::TOP_LEFT);
  button_play_next_tooltip->set_parent_anchor(Anchor::TOP_LEFT);
  button_play_next_tooltip->set_anchor(Anchor::TOP_LEFT);
  button_sort_tooltip->set_parent_anchor(Anchor::TOP_LEFT);
  button_sort_tooltip->set_anchor(Anchor::TOP_LEFT);
  button_more_tooltip->set_parent_anchor(Anchor::TOP_LEFT);
  button_more_tooltip->set_anchor(Anchor::TOP_LEFT);

  items_container = &add_child<Widget>();

  insert_cursor = &add_child<Sprite>("insert_cursor");
  insert_cursor->set_height(2);
  insert_cursor->set_is_drawn(false);
}

PanelTracks::~PanelTracks() {}

void PanelTracks::create_item_widget_if_null(Item& item) {
  size_t track_number = item.track_info.index;
  if (item.has_widget()) { return; }

  if (item.type == ItemType::TRACK) {
    auto* w = &items_container->add_child<WidgetTrack>();
    w->collection_id(item.track_info.collection_id)
      .playlist_id(item.track_info.playlist_id)
      .track_id(item.track_info.track_id)
      .track_number(track_number + 1)
      .highlight_mode(track_highlight_mode);

    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    item.widget_track = w;
    w->on_press([this, ti = item.track_info, w]() -> void {
      bool ctrl = Input::ctrl_pressed() && !Input::shift_pressed();
      bool shift = !Input::ctrl_pressed() && Input::shift_pressed();
      if (ctrl) {
        if (m_selection.has(ti)) {
          m_selection.erase(ti);
        } else {
          m_selection.insert(ti);
        }
        selection_modified = true;
      } else if (shift) {
        std::optional<size_t> index_last;
        std::optional<size_t> index_curr;
        for (size_t i = 0; i < items.size(); i += 1) {
          if (m_selection.back().has_value()) {
            if (items[i].track_info == m_selection.back()) { index_last = i; }
          } else {
            auto playing = player::get_playing();
            if (playing.has_value() && items[i].track_info.collection_id == playing->collection_id &&
                items[i].track_info.playlist_id == playing->playlist_id &&
                items[i].track_info.track_id == playing->track_id) {
              index_last = i;
            }
          }
          if (items[i].track_info == ti) { index_curr = i; }
          // if (!index_curr.has_value()) { index_curr = index_last; }
          // if (!index_last.has_value()) { index_last = index_curr; }
          if (index_curr.has_value() && index_last.has_value()) {
            for (size_t j = std::min(*index_curr, *index_last); j <= std::max(*index_curr, *index_last); j += 1) {
              if (items[j].type == ItemType::TRACK) {
                m_selection.insert(items[j].track_info);
                selection_modified = true;
              }
            }
            break;
          }
        }
      } else if (on_track_lmb) {
        on_track_lmb(ti, w);
        if (!m_selection.empty()) {
          m_selection.clear();
          selection_modified = true;
        }
      }
    });

    w->on_press_rmb([this, ti = item.track_info, w]() -> void {
      if (m_selection.has(w->track_info()) && on_selection_rmb) {
        on_selection_rmb(w);
      } else if (on_track_rmb) {
        on_track_rmb(ti, w);
      }
    });

  } else if (item.type == ItemType::HEADER) {
    auto* w = &items_container->add_child<WidgetPlaylistHeader>(collection_id.value(), item.track_info.playlist_id);
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    item.widget_header = w;
    w->button_sort->on_press([this, ti = item.track_info, w]() -> void {
      if (on_playlist_sort_button_pressed) {
        on_playlist_sort_button_pressed(ti.collection_id, ti.playlist_id, w->button_sort);
      }
    });
    w->button_more->on_press([this, ti = item.track_info, w]() -> void {
      if (on_playlist_more_options_invoked) {
        on_playlist_more_options_invoked(ti.collection_id, ti.playlist_id, w->button_more);
      }
    });
  }
}

void PanelTracks::draw() { Sprite::draw(); }

void PanelTracks::scroll_to_playlist(size_t target_playlist_id) {
  i32 offset = 0;
  for (auto& item : items) {
    if (item.type == ItemType::HEADER) {
      if (item.track_info.playlist_id == target_playlist_id) { break; }
    }
    offset += item.height();
  }

  scrollbar->set_scroll_offset(offset);
}

void PanelTracks::scroll_to_track(size_t playlist_id, size_t track_id) {
  i32 offset = 0;
  for (auto& item : items) {
    auto ti = item.track_info;
    if (item.type == ItemType::TRACK && ti.track_id == track_id && ti.playlist_id == playlist_id) { break; }
    offset += item.height();
  }

  scrollbar->set_scroll_offset(offset);
}

void PanelTracks::update() {
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

  items_container->set_pos(scrollbar->get_width(), 0);
  items_container->set_size(width - scrollbar->get_width(), height);

  std::optional<size_t> tooltip_visible_index;

  bool draw_insert_cursor = false;

  for (auto&& widget : items_container->get_children()) {
    WidgetPlaylistHeader* widget_header = dynamic_cast<WidgetPlaylistHeader*>(widget.get());
    WidgetTrack* widget_track = dynamic_cast<WidgetTrack*>(widget.get());

    if (!draw_insert_cursor && widget_track && widget_track->track_info() == insert_cursor_track_info) {
      draw_insert_cursor = true;
      if (insert_cursor_pos == InsertCursorPos::ABOVE) {
        insert_cursor->set_y(widget_track->get_y());
      } else if (insert_cursor_pos == InsertCursorPos::BELOW) {
        insert_cursor->set_y(widget_track->get_y() + widget_track->get_height());
      }
    }

    if (!widget_header) { continue; }

    if (widget_header->button_play->is_mouse_hovering()) {
      tooltip_visible_index = 0;
      button_play_tooltip->set_pos(widget_header->button_play->get_position() - get_position());
      button_play_tooltip->set_x(button_play_tooltip->get_x() - button_play_tooltip->get_width() / 2);
      button_play_tooltip->set_y(button_play_tooltip->get_y() + 26);
      break;
    }
    if (widget_header->button_play_next->is_mouse_hovering()) {
      tooltip_visible_index = 1;
      button_play_next_tooltip->set_pos(widget_header->button_play_next->get_position() - get_position());
      button_play_next_tooltip->set_x(button_play_next_tooltip->get_x() - button_play_next_tooltip->get_width() / 2);
      button_play_next_tooltip->set_y(button_play_next_tooltip->get_y() + 26);
      break;
    }
    if (widget_header->button_sort->is_mouse_hovering()) {
      tooltip_visible_index = 2;
      button_sort_tooltip->set_pos(widget_header->button_sort->get_position() - get_position());
      button_sort_tooltip->set_x(button_sort_tooltip->get_x() - button_sort_tooltip->get_width() / 2);
      button_sort_tooltip->set_y(button_sort_tooltip->get_y() + 26);
      break;
    }
    if (widget_header->button_more->is_mouse_hovering()) {
      tooltip_visible_index = 3;
      button_more_tooltip->set_pos(widget_header->button_more->get_position() - get_position());
      button_more_tooltip->set_x(button_more_tooltip->get_x() - button_more_tooltip->get_width() / 2);
      button_more_tooltip->set_y(button_more_tooltip->get_y() + 26);
      break;
    }
  }

  insert_cursor->set_is_drawn(draw_insert_cursor);

  button_play_tooltip->set_is_drawn(tooltip_visible_index == 0);
  button_play_next_tooltip->set_is_drawn(tooltip_visible_index == 1);
  button_sort_tooltip->set_is_drawn(tooltip_visible_index == 2);
  button_more_tooltip->set_is_drawn(tooltip_visible_index == 3);

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  scrollbar->set_page_size(height);
  scrollbar->set_height(height);
  if ((i32)scroll_px != (i32)old_scroll_px || vec2i{width, height} != old_size || just_recreated ||
      selection_modified) {

    insert_cursor->set_x(items_container->get_x());
    insert_cursor->set_width(items_container->get_width());

    static constexpr i32 MARGIN = 100;
    i32 current_item_top_y = 0;
    i32 track_number = 0;
    for (size_t i = 0; i < items.size(); i += 1) {
      i32 current_item_bottom_y = current_item_top_y + items[i].height();
      bool item_visible =
        current_item_top_y + MARGIN >= scroll_px && current_item_bottom_y - MARGIN <= scroll_px + height;
      if (item_visible) {
        items[i].track_info.index = track_number;
        create_item_widget_if_null(items[i]);
        ensure(items[i].widget());
        items[i].widget()->set_width(items_container->get_width());
        items[i].widget()->set_y(current_item_top_y - scroll_px);
        if (items[i].type == ItemType::HEADER) { ui.mark_dirty_recursive(items[i].widget()); }
        items[i].widget()->set_is_drawn(item_visible);
        if (items[i].type == ItemType::TRACK) {
          WidgetTrack* widget_track = items[i].widget_track;
          ensure(widget_track);
          if (widget_track) { widget_track->is_selected(m_selection.has(items[i].track_info)); }
        }
      } else if (items[i].widget()) {
        items[i].delete_widget();
      }

      current_item_top_y += items[i].height();
      track_number += 1;
      if (items[i].type == ItemType::HEADER) { track_number = 0; }
      ui.mark_dirty_recursive(items_container); // FIXME without this the view flickers when scrolling down
    }
    old_scroll_px = scroll_px;
    old_size = vec2i{width, height};
    just_recreated = false;
    selection_modified = false;
  }

  Sprite::update();
}

void PanelTracks::event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelTracks::event(Input::InputEventMouseButton& e) {
  if (is_mouse_hovering()) {
    if (!m_selection.empty() && e.button == Input::MouseButton::MOUSE_BUTTON_LEFT &&
        e.action == Input::MouseAction::RELEASE) {
      m_selection.clear();
      selection_modified = true;
    }
    e.handled = true;
  }
}

void PanelTracks::clear() {
  collection_id = std::nullopt;
  items.clear();
  for (auto&& widget : items_container->get_children()) {
    widget->set_marked_for_deletion(true);
  }
  max_scroll_px = 0;
  just_recreated = true;
  m_selection.clear();
  scrollbar->set_content_size(0);
}

float PanelTracks::get_scroll_px() const { return target_scroll_px; }

void PanelTracks::set_scroll_px(float px) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
}

void PanelTracks::recreate(std::optional<size_t> collection_id_) {
  clear();
  collection_id = collection_id_;
  if (!collection_id.has_value()) { return; }

  auto& collection = db::collection_by_id(*collection_id)->get();
  for (auto playlist_id : collection.playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }
    Item item_header = {
      .type = ItemType::HEADER,
      .track_info = {.collection_id = collection_id.value(), .playlist_id = playlist_id, .track_id = db::INVALID_ID}};
    max_scroll_px += item_header.height();
    items.emplace_back(item_header);
    for (auto track_id : playlist.track_ids) {
      Item item_track = {
        .type = ItemType::TRACK,
        .track_info = {.collection_id = collection_id.value(), .playlist_id = playlist_id, .track_id = track_id}};
      max_scroll_px += item_track.height();
      items.emplace_back(item_track);
    }
  }

  scrollbar->set_content_size(max_scroll_px);
}

void PanelTracks::recreate(std::span<const db::track_info> tracks) {
  clear();
  collection_id = std::nullopt;

  for (auto& ti : tracks) {
    Item item_track = {.type = ItemType::TRACK, .track_info = ti};
    max_scroll_px += item_track.height();
    items.emplace_back(item_track);
  }

  scrollbar->set_content_size(max_scroll_px);
}

void PanelTracks::insert_track(size_t index, db::track_info ti) {
  index = std::min(index, items.size());
  Item item_track = {.type = ItemType::TRACK, .track_info = ti};
  items.insert(items.begin() + index, item_track);
  max_scroll_px += item_track.height();
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
}

void PanelTracks::insert_track(db::track_info ti) { insert_track(items.size(), ti); }

void PanelTracks::set_track(size_t index, db::track_info ti) {
  if (index >= items.size()) { return; }
  i32 prev_height = items[index].height();
  Item item_track = {.type = ItemType::TRACK, .track_info = ti};
  items[index] = item_track;
  max_scroll_px += (item_track.height() - prev_height);
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
}
