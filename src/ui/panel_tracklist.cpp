#include "panel_tracklist.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include "common/color.hpp"
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "theme.hpp"
#include "ui/tr.hpp"
#include "ui/widget_queue_playlist_collection.hpp"
#include "ui/widget_track.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "widget_playlist_header.hpp"
#include "zincbox.hpp"

using namespace zincgui;

i32 PanelTracklist::get_item_height(const Item& item) {
  return std::visit([](const auto& i) { return i.height(); }, item);
}

bool PanelTracklist::is_item_selectable(const Item& item) {
  return std::visit([](const auto& i) { return std::decay_t<decltype(i)>::is_selectable; }, item);
}

zincgui::Widget* PanelTracklist::get_item_widget(Item& item) {
  return std::visit([](auto& i) -> zincgui::Widget* { return i.widget; }, item);
}

void PanelTracklist::delete_item_widget(Item& item) {
  std::visit(
    [](auto& i) {
      if (i.widget) {
        i.widget->set_marked_for_deletion(true);
        i.widget = nullptr;
      }
    },
    item);
}

size_t PanelTracklist::get_item_index(const Item& item) {
  return std::visit([](const auto& i) { return i.item_index; }, item);
}

void PanelTracklist::set_item_index(Item& item, size_t index) {
  std::visit([index](auto& i) { i.item_index = index; }, item);
}

void PanelTracklist::set_item_selected(Item& item, bool selected) {
  std::visit(
    [selected](auto& i) -> void {
      if constexpr (i.is_selectable) {
        if (i.widget) { i.widget->is_selected(selected); }
      }
    },
    item);
}

std::optional<size_t> PanelTracklist::find_item_track(db::track_info ti) const {
  auto it = track_lookup.find(ti);
  if (it != track_lookup.end()) { return it->second; }
  return std::nullopt;
}

void PanelTracklist::rebuild_lookup() {
  track_lookup.clear();
  for (size_t i = 0; i < items.size(); ++i) {
    if (auto* t = std::get_if<ItemTrack>(&items[i])) { track_lookup[t->track_info] = i; }
  }
}

PanelTracklist::PanelTracklist(Root& ui_) : ColorRect(ui_) {
  set_color(theme::config().panel_tracklist.color);
  set_clip_children(true);

  scrollbar = &add_child<ZincboxScrollbar>();
  scrollbar->set_anchor(Anchor::LEFT);
  scrollbar->set_parent_anchor(Anchor::LEFT);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });
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

  insert_cursor = &add_child<ColorRect>(theme::config().panel_tracklist.insert_cursor_color);
  insert_cursor->set_height(2);
  insert_cursor->set_anchor(Anchor::CENTER_LEFT);
  insert_cursor->set_is_drawn(false);
}

PanelTracklist::~PanelTracklist() {}

void PanelTracklist::create_item_widget_if_null(Item& item) {
  if (get_item_widget(item)) { return; }

  zincgui::Widget* generic_widget = nullptr;

  if (auto* t = std::get_if<ItemTrack>(&item)) {
    auto* w = &items_container->add_child<WidgetTrack>();
    w->collection_id(t->track_info.collection_id)
      .playlist_id(t->track_info.playlist_id)
      .track_id(t->track_info.track_id)
      .track_number(t->track_info.index + 1)
      .item_index(t->item_index)
      .highlight_mode(track_highlight_mode);

    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    t->widget = w;
    generic_widget = w;
  } else if (auto* qt = std::get_if<ItemQueueTrack>(&item)) {
    auto* w = &items_container->add_child<WidgetTrack>();
    if (qt->queue_index < player::get_playing_queue().size()) {
      auto q_elem = player::get_playing_queue()[qt->queue_index];
      if (q_elem.is_track()) {
        w->collection_id(q_elem.track()->track_info.collection_id)
          .playlist_id(q_elem.track()->track_info.playlist_id)
          .track_id(q_elem.track()->track_info.track_id)
          .track_number(qt->queue_index + 1)
          .item_index(qt->item_index)
          .highlight_mode(track_highlight_mode);
      }
    }
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    qt->widget = w;
    generic_widget = w;
  } else if (auto* qp = std::get_if<ItemQueuePlaylist>(&item)) {
    auto* w = &items_container->add_child<WidgetQueuePlaylistCollection>();
    if (qp->queue_index < player::get_playing_queue().size()) {
      auto q_elem = player::get_playing_queue()[qp->queue_index];
      if (q_elem.is_playlist()) {
        w->collection_id(q_elem.playlist()->get_collection_id())
          .playlist_id(q_elem.playlist()->get_playlist_id().value_or(db::INVALID_ID))
          .queue_index(qp->queue_index)
          .track_current(q_elem.playlist()->get_playback_index().value_or(0))
          .tracks_total(q_elem.playlist()->get_size())
          .item_type(WidgetQueuePlaylistCollection::ItemType::PLAYLIST);
      }
    }
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    qp->widget = w;
    generic_widget = w;
  } else if (auto* qc = std::get_if<ItemQueueCollection>(&item)) {
    auto* w = &items_container->add_child<WidgetQueuePlaylistCollection>();
    if (qc->queue_index < player::get_playing_queue().size()) {
      auto q_elem = player::get_playing_queue()[qc->queue_index];
      if (q_elem.is_collection()) {
        w->collection_id(q_elem.collection()->get_collection_id())
          .queue_index(qc->queue_index)
          .track_current(q_elem.collection()->get_playback_index().value_or(0))
          .tracks_total(q_elem.collection()->get_size())
          .item_type(WidgetQueuePlaylistCollection::ItemType::COLLECTION);
      }
    }
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    qc->widget = w;
    generic_widget = w;
  } else if (auto* h = std::get_if<ItemPlaylistHeader>(&item)) {
    auto* w = &items_container->add_child<WidgetPlaylistHeader>(h->collection_id, h->playlist_id);
    w->set_ignore_parents_layout(true);
    w->set_is_drawn(false);
    h->widget = w;
    w->button_sort->on_press([this, collection_id = h->collection_id, playlist_id = h->playlist_id, w]() -> void {
      if (on_playlist_sort_button_pressed) {
        on_playlist_sort_button_pressed(collection_id, playlist_id, w->button_sort);
      }
    });
    w->button_more->on_press([this, collection_id = h->collection_id, playlist_id = h->playlist_id, w]() -> void {
      if (on_playlist_more_options_invoked) {
        on_playlist_more_options_invoked(collection_id, playlist_id, w->button_more);
      }
    });
    w->update();
  }

  size_t item_index = 0;
  std::visit([&item_index](auto& i) { item_index = i.item_index; }, item);

  if (generic_widget) {
    auto handle_press = [item_index, this, generic_widget]() -> void {
      bool ctrl = Input::ctrl_pressed() && !Input::shift_pressed();
      bool shift = !Input::ctrl_pressed() && Input::shift_pressed();

      if (ctrl) {
        if (m_selection.has(item_index)) {
          m_selection.erase(item_index);
        } else {
          m_selection.insert(item_index);
        }
        selection_modified = true;
      } else if (shift) {
        std::optional<size_t> index_last;
        if (m_selection.back().has_value()) {
          index_last = m_selection.back();
        } else {
          auto playing_track = player::get_playing_track();
          if (playing_track.has_value()) { index_last = find_item_track(playing_track.value()); }
        }

        if (index_last.has_value()) {
          if (item_index < *index_last) {
            for (size_t j = item_index; j <= *index_last; j += 1) {
              if (is_item_selectable(items[j]) && m_selection.insert(j)) { selection_modified = true; }
            }
          } else {
            for (size_t j = *index_last; j <= item_index; j += 1) {
              if (is_item_selectable(items[j]) && m_selection.insert(j)) { selection_modified = true; }
            }
          }
        }
      } else if (on_item_lmb) {
        on_item_lmb(item_index);
        if (m_selection.clear()) { selection_modified = true; }
      }
    };

    auto handle_press_rmb = [item_index, this, generic_widget]() -> void {
      if (!m_selection.empty()) {
        if (m_selection.has(item_index) && on_selection_rmb) {
          on_selection_rmb(item_index);
        } else if (on_item_rmb) {
          clear_selection();
          on_item_rmb(item_index);
        }
      } else if (on_item_rmb) {
        on_item_rmb(item_index);
      }
    };

    if (auto* w = dynamic_cast<WidgetTrack*>(generic_widget)) {
      w->on_press(handle_press);
      w->on_press_rmb(handle_press_rmb);
      w->update();
    } else if (auto* w2 = dynamic_cast<WidgetQueuePlaylistCollection*>(generic_widget)) {
      w2->on_press(handle_press);
      w2->on_press_rmb(handle_press_rmb);
      w2->update();
    }
  }
}

void PanelTracklist::draw() { ColorRect::draw(); }

void PanelTracklist::show() {
  set_is_drawn(true);
  set_is_updated(true);
  input();
  update();
  scrollbar->input();
  scrollbar->update();
}

void PanelTracklist::hide() {
  set_is_drawn(false);
  set_is_updated(false);
}

void PanelTracklist::scroll_to_playlist(size_t target_playlist_id, bool immediate) {
  i32 offset = 0;
  for (auto& item : items) {
    if (auto* h = std::get_if<ItemPlaylistHeader>(&item)) {
      if (h->playlist_id == target_playlist_id) { break; }
    }
    offset += get_item_height(item);
  }
  target_scroll_px = offset;
  target_scroll_px = std::clamp<double>(target_scroll_px, 0.0, std::max<double>(0.0, max_scroll_px - height));
  scrollbar->set_scroll_offset(target_scroll_px);
  if (immediate) {
    scroll_px = target_scroll_px;
    scrollbar->skip_anim();
  }
}

void PanelTracklist::scroll_to_track(size_t playlist_id, size_t track_id, bool immediate) {
  i32 offset = 0;
  for (auto& item : items) {
    if (auto* t = std::get_if<ItemTrack>(&item)) {
      if (t->track_info.track_id == track_id && t->track_info.playlist_id == playlist_id) { break; }
    }
    offset += get_item_height(item);
  }
  target_scroll_px = std::clamp<double>(offset, 0.0, std::max<double>(0.0, max_scroll_px - height));
  scrollbar->set_scroll_offset(target_scroll_px);
  if (immediate) {
    scroll_px = target_scroll_px;
    scrollbar->skip_anim();
  }
}

void PanelTracklist::input() {
  items_container->set_pos(scrollbar->get_width(), 0);
  items_container->set_size(width - scrollbar->get_width(), height);

  // scroll on the edges of view when dragging
  if (is_dragged && is_mouse_hovering()) {
    static i32 THRESHOLD = 50 * zincbox::ui_scale();
    i32 mouse_y = Input::get_mouse_y() - get_position().y;

    if (mouse_y < THRESHOLD) {
      scroll_edge_factor = (mouse_y - THRESHOLD) / (float)THRESHOLD;
    } else if (mouse_y > height - THRESHOLD && mouse_y < height) {
      scroll_edge_factor = (THRESHOLD - (height - mouse_y)) / (float)THRESHOLD;
    } else {
      scroll_edge_factor *= 0.5f;
    }
  } else {
    scroll_edge_factor *= 0.5f;
  }

  if (std::abs(scroll_edge_factor) > 0.01f) {
    scroll_edge_factor = std::clamp(scroll_edge_factor, -1.0f, 1.0f);
    scrollbar->set_scroll_offset(target_scroll_px + (double)scroll_edge_factor * 8.0);
  }

  if ((i32)scroll_px != (i32)old_scroll_px || vec2i{width, height} != old_size || just_recreated ||
      selection_modified) {

    insert_cursor->set_x(items_container->get_x());
    insert_cursor->set_width(items_container->get_width());

    static constexpr i32 MARGIN = 100;
    i32 current_item_top_y = 0;
    for (size_t i = 0; i < items.size(); i += 1) {
      i32 item_h = get_item_height(items[i]);
      i32 current_item_bottom_y = current_item_top_y + item_h;
      bool item_visible =
        current_item_top_y + MARGIN >= scroll_px && current_item_bottom_y - MARGIN <= scroll_px + height;
      if (item_visible) {
        set_item_index(items[i], i);
        create_item_widget_if_null(items[i]);
        ensure(get_item_widget(items[i]));
        get_item_widget(items[i])->set_width(items_container->get_width());
        get_item_widget(items[i])->set_y(current_item_top_y - scroll_px);
        if (std::holds_alternative<ItemPlaylistHeader>(items[i])) {
          ui.mark_dirty_recursive(get_item_widget(items[i]));
        }
        get_item_widget(items[i])->set_is_drawn(item_visible);
        set_item_selected(items[i], m_selection.has(i));
      } else if (get_item_widget(items[i])) {
        delete_item_widget(items[i]);
      }

      current_item_top_y += item_h;
      ui.mark_dirty_recursive(items_container); // FIXME without this the view flickers when scrolling down
    }
    old_scroll_px = scroll_px;
    old_size = vec2i{width, height};
    just_recreated = false;
    selection_modified = false;
  }
}

void PanelTracklist::update() {
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(max_scroll_px - get_height())));

  std::optional<size_t> tooltip_visible_index;

  bool draw_insert_cursor = false;

  for (auto&& widget : items_container->get_children()) {
    WidgetPlaylistHeader* widget_header = dynamic_cast<WidgetPlaylistHeader*>(widget.get());
    WidgetTrack* widget_track = dynamic_cast<WidgetTrack*>(widget.get());

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

  button_play_tooltip->set_is_drawn(tooltip_visible_index == 0);
  button_play_next_tooltip->set_is_drawn(tooltip_visible_index == 1);
  button_sort_tooltip->set_is_drawn(tooltip_visible_index == 2);
  button_more_tooltip->set_is_drawn(tooltip_visible_index == 3);

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  if (!draw_insert_cursor && insert_cursor_index.has_value() && *insert_cursor_index < items.size()) {
    insert_cursor->set_is_drawn(true);
    if (auto* w = get_item_widget(items[*insert_cursor_index])) {
      draw_insert_cursor = true;
      if (insert_cursor_pos == InsertCursorPos::ABOVE) {
        insert_cursor->set_y(w->get_y());
      } else if (insert_cursor_pos == InsertCursorPos::BELOW) {
        insert_cursor->set_y(w->get_y() + w->get_height());
      }
    }
  } else {
    insert_cursor->set_is_drawn(false);
  }

  scrollbar->set_page_size(height);
  scrollbar->set_height(height);
  ColorRect::update();
}

void PanelTracklist::event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelTracklist::event(Input::InputEventMouseButton& e) {
  if (is_mouse_hovering()) {
    if (!m_selection.empty() && e.button == Input::MouseButton::MOUSE_BUTTON_LEFT &&
        e.action == Input::MouseAction::PRESS) {
      m_selection.clear();
      selection_modified = true;
    }
    e.handled = true;
  }
}

void PanelTracklist::event(Input::InputEventKey& e) {
  if (e.key == Input::Key::KEY_ESCAPE && e.action == Input::KeyAction::PRESS) {
    m_selection.clear();
    selection_modified = true;
  }
}

void PanelTracklist::clear() {
  collection_id = std::nullopt;
  items.clear();
  for (auto&& widget : items_container->get_children()) {
    widget->set_marked_for_deletion(true);
  }
  max_scroll_px = 0;
  just_recreated = true;
  m_selection.clear();
  scrollbar->set_content_size(0);
  rebuild_lookup();
}

float PanelTracklist::get_scroll_px() const { return target_scroll_px; }

void PanelTracklist::set_scroll_px(float px, bool immediate) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
  if (immediate) { scrollbar->skip_anim(); }
}

void PanelTracklist::recreate(std::optional<size_t> collection_id_) {
  clear();
  collection_id = collection_id_;
  if (!collection_id.has_value()) { return; }

  auto& collection = db::collection_by_id(*collection_id)->get();
  for (auto playlist_id : collection.playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }
    ItemPlaylistHeader item_header;
    item_header.collection_id = *collection_id;
    item_header.playlist_id = playlist_id;
    item_header.item_index = items.size();
    max_scroll_px += item_header.height();
    items.emplace_back(item_header);
    for (size_t track_index = 0; track_index < playlist.track_ids.size(); track_index += 1) {
      auto track_id = playlist.track_ids[track_index];
      db::track_info track_info = {
        .collection_id = *collection_id, .playlist_id = playlist_id, .track_id = track_id, .index = track_index};
      ItemTrack item_track;
      item_track.track_info = track_info;
      item_track.item_index = items.size();
      max_scroll_px += item_track.height();
      items.emplace_back(item_track);
    }
  }

  scrollbar->set_content_size(max_scroll_px);
  set_scroll_px(scroll_px);
  rebuild_lookup();
}

void PanelTracklist::recreate(std::span<const db::track_info> tracks) {
  clear();
  collection_id = std::nullopt;

  for (auto& ti : tracks) {
    ItemTrack item_track;
    item_track.track_info = ti;
    item_track.item_index = items.size();
    max_scroll_px += item_track.height();
    items.emplace_back(item_track);
  }

  scrollbar->set_content_size(max_scroll_px);
  set_scroll_px(scroll_px);
  rebuild_lookup();
}

void PanelTracklist::insert_track(size_t index, db::track_info ti) {
  index = std::min(index, items.size());
  ItemTrack item_track;
  item_track.track_info = ti;
  item_track.item_index = index;
  items.insert(items.begin() + index, item_track);
  for (size_t i = index; i < items.size(); i += 1) {
    set_item_index(items[i], i);
  }
  max_scroll_px += item_track.height();
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::insert_track(db::track_info ti) { insert_track(items.size(), ti); }

void PanelTracklist::recreate(std::span<const Item> new_items) {
  clear();
  collection_id = std::nullopt;

  for (auto& new_item : new_items) {
    Item item = new_item;
    set_item_index(item, items.size());
    max_scroll_px += get_item_height(item);
    items.emplace_back(item);
  }

  scrollbar->set_content_size(max_scroll_px);
  set_scroll_px(scroll_px);
  rebuild_lookup();
}

void PanelTracklist::insert_item(size_t index, const Item& item) {
  index = std::min(index, items.size());
  Item new_item = item;
  set_item_index(new_item, index);
  items.insert(items.begin() + index, new_item);
  for (size_t i = index; i < items.size(); i += 1) {
    set_item_index(items[i], i);
  }
  max_scroll_px += get_item_height(new_item);
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::insert_item(const Item& item) { insert_item(items.size(), item); }

void PanelTracklist::set_item(size_t index, const Item& item) {
  if (index >= items.size()) { return; }
  i32 prev_height = get_item_height(items[index]);
  Item new_item = item;
  set_item_index(new_item, index);
  items[index] = new_item;
  max_scroll_px += (get_item_height(new_item) - prev_height);
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::insert_to_selection_index(size_t index) {
  if (index < items.size()) {
    if (m_selection.insert(index)) { selection_modified = true; }
  }
}

void PanelTracklist::set_track(size_t index, db::track_info ti) {
  if (index >= items.size()) { return; }
  i32 prev_height = get_item_height(items[index]);
  ItemTrack item_track;
  item_track.track_info = ti;
  item_track.item_index = index;
  items[index] = item_track;
  max_scroll_px += (item_track.height() - prev_height);
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::remove_item(size_t index) {
  if (index >= items.size()) { return; }
  i32 h = get_item_height(items[index]);
  items.erase(items.begin() + index);
  for (size_t i = index; i < items.size(); i += 1) {
    set_item_index(items[i], i);
  }
  max_scroll_px -= h;
  scrollbar->set_content_size(max_scroll_px);
  just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}
