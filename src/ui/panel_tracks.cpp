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

  elements_container = &add_child<Widget>();

  insert_cursor = &add_child<Sprite>("insert_cursor");
  insert_cursor->set_height(2);
  insert_cursor->set_is_drawn(false);
}

PanelTracks::~PanelTracks() {}

void PanelTracks::create_element(std::pair<element, Widget*>& e) {
  auto& [element, widget] = e;
  size_t track_number = element.track_info.index;
  if (widget) { return; }

  if (element.type == ElementType::TRACK) {
    auto* w = &elements_container->add_child<WidgetTrack>();
    w->collection_id(element.track_info.collection_id)
      .playlist_id(element.track_info.playlist_id)
      .track_id(element.track_info.track_id)
      .track_number(track_number + 1)
      .highlight_mode(track_highlight_mode);

    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    widget = w;
    w->on_press([this, element, w]() -> void {
      bool ctrl = Input::ctrl_pressed() && !Input::shift_pressed();
      bool shift = !Input::ctrl_pressed() && Input::shift_pressed();
      if (ctrl) {
        if (m_selection.has(element.track_info)) {
          m_selection.erase(element.track_info);
        } else {
          m_selection.insert(element.track_info);
        }
        selection_modified = true;
      } else if (shift) {
        std::optional<size_t> index_last;
        std::optional<size_t> index_curr;
        for (size_t i = 0; i < elements.size(); i += 1) {
          if (m_selection.back().has_value()) {
            if (elements[i].first.track_info == m_selection.back()) { index_last = i; }
          } else {
            auto playing = player::get_playing();
            if (playing.has_value() && elements[i].first.track_info.collection_id == playing->collection_id &&
                elements[i].first.track_info.playlist_id == playing->playlist_id &&
                elements[i].first.track_info.track_id == playing->track_id) {
              index_last = i;
            }
          }
          if (elements[i].first.track_info == element.track_info) { index_curr = i; }
          // if (!index_curr.has_value()) { index_curr = index_last; }
          // if (!index_last.has_value()) { index_last = index_curr; }
          if (index_curr.has_value() && index_last.has_value()) {
            for (size_t j = std::min(*index_curr, *index_last); j <= std::max(*index_curr, *index_last); j += 1) {
              if (elements[j].first.type == ElementType::TRACK) {
                m_selection.insert(elements[j].first.track_info);
                selection_modified = true;
              }
            }
            break;
          }
        }
      } else if (on_track_lmb) {
        on_track_lmb(element.track_info, w);
        if (!m_selection.empty()) {
          m_selection.clear();
          selection_modified = true;
        }
      }
    });

    w->on_press_rmb([this, element, w]() -> void {
      if (m_selection.has(w->track_info()) && on_selection_rmb) {
        on_selection_rmb(w);
      } else if (on_track_rmb) {
        on_track_rmb(element.track_info, w);
      }
    });

  } else if (element.type == ElementType::HEADER) {
    auto* w =
      &elements_container->add_child<WidgetPlaylistHeader>(collection_id.value(), element.track_info.playlist_id);
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    widget = w;
    w->button_sort->on_press([this, element, w]() -> void {
      if (on_playlist_sort_button_pressed) {
        on_playlist_sort_button_pressed(element.track_info.collection_id, element.track_info.playlist_id,
                                        w->button_sort);
      }
    });
    w->button_more->on_press([this, element, w]() -> void {
      if (on_playlist_more_options_invoked) {
        on_playlist_more_options_invoked(element.track_info.collection_id, element.track_info.playlist_id,
                                         w->button_more);
      }
    });
  }
}

void PanelTracks::draw() { Sprite::draw(); }

void PanelTracks::scroll_to_playlist(size_t target_playlist_id) {
  static i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
  static i32 header_height = theme::get_prop("tracklist_playlist_header_height").as_i32();
  i32 offset = 0;
  for (auto& [element, _] : elements) {
    if (element.type == ElementType::TRACK) {
      offset += track_height;
    } else if (element.type == ElementType::HEADER) {
      if (element.track_info.playlist_id == target_playlist_id) { break; }
      offset += header_height;
    }
  }

  scrollbar->set_scroll_offset(offset);
}

void PanelTracks::scroll_to_track(size_t target_playlist_id, size_t target_track_id) {
  static i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
  static i32 header_height = theme::get_prop("tracklist_playlist_header_height").as_i32();
  i32 offset = 0;
  for (auto& [element, _] : elements) {
    if (element.type == ElementType::TRACK) {
      if (element.track_info.track_id == target_track_id && element.track_info.playlist_id == target_playlist_id) {
        break;
      }
      offset += track_height;
    } else if (element.type == ElementType::HEADER) {
      offset += header_height;
    }
  }

  scrollbar->set_scroll_offset(offset);
}

void PanelTracks::update() {
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

  elements_container->set_pos(scrollbar->get_width(), 0);
  elements_container->set_size(width - scrollbar->get_width(), height);

  std::optional<size_t> tooltip_visible_index;

  bool draw_insert_cursor = false;

  for (auto&& widget : elements_container->get_children()) {
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

    insert_cursor->set_x(elements_container->get_x());
    insert_cursor->set_width(elements_container->get_width());

    static constexpr i32 MARGIN = 100;
    i32 current_element_top_y = 0;
    i32 track_number = 0;
    for (size_t i = 0; i < elements.size(); i += 1) {
      i32 current_element_bottom_y = current_element_top_y + elements[i].first.height();
      bool element_visible =
        current_element_top_y + MARGIN >= scroll_px && current_element_bottom_y - MARGIN <= scroll_px + height;
      if (element_visible) {
        elements[i].first.track_info.index = track_number;
        create_element(elements[i]);
        elements[i].second->set_width(elements_container->get_width());
        elements[i].second->set_y(current_element_top_y - scroll_px);
        if (elements[i].first.type == ElementType::HEADER) { ui.mark_dirty_recursive(elements[i].second); }
        elements[i].second->set_is_drawn(element_visible);
        if (elements[i].first.type == ElementType::TRACK) {
          WidgetTrack* widget_track = dynamic_cast<WidgetTrack*>(elements[i].second);
          if (widget_track) { widget_track->is_selected(m_selection.has(elements[i].first.track_info)); }
        }
      } else if (elements[i].second) {
        elements[i].second->set_marked_for_deletion(true);
        elements[i].second = nullptr;
      }

      current_element_top_y += elements[i].first.height();
      track_number += 1;
      if (elements[i].first.type == ElementType::HEADER) { track_number = 0; }
      ui.mark_dirty_recursive(elements_container); // FIXME without this the view flickers when scrolling down
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
  elements.clear();
  for (auto&& widget : elements_container->get_children()) {
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
    element element_header = {
      .type = ElementType::HEADER,
      .track_info = {.collection_id = collection_id.value(), .playlist_id = playlist_id, .track_id = db::INVALID_ID}};
    max_scroll_px += element_header.height();
    elements.emplace_back(element_header, nullptr);
    for (auto track_id : playlist.track_ids) {
      element element_track = {
        .type = ElementType::TRACK,
        .track_info = {.collection_id = collection_id.value(), .playlist_id = playlist_id, .track_id = track_id}};
      max_scroll_px += element_track.height();
      elements.emplace_back(element_track, nullptr);
    }
  }

  scrollbar->set_content_size(max_scroll_px);
}

void PanelTracks::recreate(std::span<const db::track_info> tracks) {
  clear();
  collection_id = std::nullopt;

  for (auto& ti : tracks) {
    element element_track = {.type = ElementType::TRACK, .track_info = ti};
    max_scroll_px += element_track.height();
    elements.emplace_back(element_track, nullptr);
  }

  scrollbar->set_content_size(max_scroll_px);
}

void PanelTracks::insert_track(size_t index, db::track_info ti) {
  index = std::min(index, elements.size());
  element element_track = {.type = ElementType::TRACK, .track_info = ti};
  elements.insert(elements.begin() + index, {element_track, nullptr});
  max_scroll_px += element_track.height();
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
}

void PanelTracks::insert_track(db::track_info ti) { insert_track(elements.size(), ti); }

void PanelTracks::set_track(size_t index, db::track_info ti) {
  if (index >= elements.size()) { return; }
  i32 prev_height = elements[index].first.height();
  element element_track = {.type = ElementType::TRACK, .track_info = ti};
  elements[index] = {element_track, nullptr};
  max_scroll_px += (element_track.height() - prev_height);
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
}
