#include "panel_tracklist.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include "common/color.hpp"
#include "common/debug.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "theme.hpp"
#include "ui/tr.hpp"
#include "ui/widget_queue_playlist_collection.hpp"
#include "ui/widget_track.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/widget.hpp"
#include "widget_playlist_header.hpp"
#include "zincbox.hpp"

using namespace zincgui;

PanelTracklist::PanelTracklist(Root& ui_) : ColorRect(ui_) {
  color(theme::config().panel_tracklist.color)
    .clip_children(true);

  m_scrollbar = &add_child<zincgui::ScrollBar>();
  m_scrollbar->anchor(Anchor::LEFT)
    .parent_anchor(Anchor::LEFT)
    .orientation(SliderOrientation::VERTICAL)
    .on_value_changed([this](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

  m_items_container = &add_child<Widget>();

  m_insert_cursor = &add_child<ColorRect>(theme::config().panel_tracklist.insert_cursor_color);
  m_insert_cursor->height(2)
    .anchor(Anchor::CENTER_LEFT)
    .is_drawn(false);
}

PanelTracklist::~PanelTracklist() {}

void PanelTracklist::input() {}

void PanelTracklist::update() {
  m_items_container->pos(m_scrollbar->width(), 0)
    .size(width() - m_scrollbar->width(), height());

  // scroll on the edges of view when dragging
  if (m_is_dragged && is_mouse_hovering()) {
    const i32 THRESHOLD = 50 * zincbox::ui_scale();
    i32 mouse_y = Input::get_mouse_y() - position().y;

    if (mouse_y < THRESHOLD) {
      m_scroll_edge_factor = (mouse_y - THRESHOLD) / static_cast<float>(THRESHOLD);
    } else if (mouse_y > height() - THRESHOLD && mouse_y < height()) {
      m_scroll_edge_factor = (THRESHOLD - (height() - mouse_y)) / static_cast<float>(THRESHOLD);
    } else {
      m_scroll_edge_factor *= 0.5f;
    }
  } else {
    m_scroll_edge_factor *= 0.5f;
  }

  if (std::abs(m_scroll_edge_factor) > 0.01f) {
    m_scroll_edge_factor = std::clamp(m_scroll_edge_factor, -1.0f, 1.0f);
    m_scrollbar->scroll_offset(m_target_scroll_px + (static_cast<double>(m_scroll_edge_factor) * 8.0));
  }

  if (static_cast<i32>(m_scroll_px) != static_cast<i32>(m_old_scroll_px) || size() != m_old_size ||
      m_just_recreated || m_selection_modified) {

    m_insert_cursor->x(m_items_container->x())
      .width(m_items_container->width());

    static constexpr i32 MARGIN = 100;
    i32 current_item_top_y = 0;
    for (size_t i = 0; i < m_items.size(); i += 1) {
      i32 item_h = get_item_height(m_items[i]);
      i32 current_item_bottom_y = current_item_top_y + item_h;
      bool item_visible =
        current_item_top_y + MARGIN >= m_scroll_px && current_item_bottom_y - MARGIN <= m_scroll_px + height();
      if (item_visible) {
        set_item_index(m_items[i], i);
        create_item_widget_if_null(m_items[i]);
        auto* item_widget = get_item_widget(m_items[i]);
        ensure(item_widget);
        item_widget->width(m_items_container->width())
          .y(current_item_top_y - m_scroll_px)
          .is_drawn(item_visible);
        set_item_selected(m_items[i], m_selection.has(i));
      } else if (get_item_widget(m_items[i])) {
        delete_item_widget(m_items[i]);
      }

      current_item_top_y += item_h;
    }
    m_old_scroll_px = m_scroll_px;
    m_old_size = size();
    m_just_recreated = false;
    m_selection_modified = false;
  }
  m_scroll_px = std::clamp(m_scroll_px, 0.0, std::max(0.0, static_cast<double>(m_max_scroll_px - height())));

  bool draw_insert_cursor = false;

  double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
  m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);

  if (!draw_insert_cursor && m_insert_cursor_index.has_value() && *m_insert_cursor_index < m_items.size()) {
    m_insert_cursor->is_drawn(true);
    if (auto* w = get_item_widget(m_items[*m_insert_cursor_index])) {
      draw_insert_cursor = true;
      if (m_insert_cursor_pos == InsertCursorPos::ABOVE) {
        m_insert_cursor->y(w->y());
      } else if (m_insert_cursor_pos == InsertCursorPos::BELOW) {
        m_insert_cursor->y(w->y() + w->height());
      }
    }
  } else {
    m_insert_cursor->is_drawn(false);
  }

  m_scrollbar->page_size(height())
    .height(height());
  ColorRect::update();
}

void PanelTracklist::event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    m_scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelTracklist::event(Input::InputEventMouseButton& e) {
  if (is_mouse_hovering()) {
    if (!m_selection.empty() && e.button == Input::MouseButton::MOUSE_BUTTON_LEFT &&
        e.action == Input::MouseAction::PRESS) {
      m_selection.clear();
      m_selection_modified = true;
    }
    e.handled = true;
  }
}

void PanelTracklist::event(Input::InputEventKey& e) {
  if (e.key == Input::Key::KEY_ESCAPE && e.action == Input::KeyAction::PRESS) {
    m_selection.clear();
    m_selection_modified = true;
  }
}

void PanelTracklist::show() { is_drawn(true).is_updated(true); }

void PanelTracklist::hide() { is_drawn(false).is_updated(false); }

void PanelTracklist::recreate(std::optional<size_t> collection_id_) {
  clear();
  m_collection_id = collection_id_;
  if (!m_collection_id.has_value()) { return; }

  auto& collection = db::collection_by_id(*m_collection_id)->get();
  for (auto playlist_id : collection.playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }
    ItemPlaylistHeader item_header;
    item_header.collection_id = *m_collection_id;
    item_header.playlist_id = playlist_id;
    item_header.item_index = m_items.size();
    m_max_scroll_px += item_header.height();
    m_items.emplace_back(item_header);
    for (size_t track_index = 0; track_index < playlist.track_ids.size(); track_index += 1) {
      auto track_id = playlist.track_ids[track_index];
      db::track_info track_info = {
        .collection_id = *m_collection_id, .playlist_id = playlist_id, .track_id = track_id, .index = track_index};
      ItemTrack item_track;
      item_track.track_info = track_info;
      item_track.item_index = m_items.size();
      m_max_scroll_px += item_track.height();
      m_items.emplace_back(item_track);
    }
  }

  m_scrollbar->content_size(m_max_scroll_px);
  scroll_px(m_scroll_px);
  rebuild_lookup();
}

void PanelTracklist::recreate(std::span<const db::track_info> tracks) {
  clear();
  m_collection_id = std::nullopt;

  for (auto& ti : tracks) {
    ItemTrack item_track;
    item_track.track_info = ti;
    item_track.item_index = m_items.size();
    m_max_scroll_px += item_track.height();
    m_items.emplace_back(item_track);
  }

  m_scrollbar->content_size(m_max_scroll_px);
  scroll_px(m_scroll_px);
  rebuild_lookup();
}

void PanelTracklist::recreate(std::span<const Item> new_items) {
  clear();
  m_collection_id = std::nullopt;

  for (auto& new_item : new_items) {
    Item item = new_item;
    set_item_index(item, m_items.size());
    m_max_scroll_px += get_item_height(item);
    m_items.emplace_back(item);
  }

  m_scrollbar->content_size(m_max_scroll_px);
  scroll_px(m_scroll_px);
  rebuild_lookup();
}

void PanelTracklist::insert_track(size_t index, db::track_info ti) {
  index = std::min(index, m_items.size());
  ItemTrack item_track;
  item_track.track_info = ti;
  item_track.item_index = index;
  m_items.insert(m_items.begin() + index, item_track);
  for (size_t i = index; i < m_items.size(); i += 1) {
    set_item_index(m_items[i], i);
  }
  m_max_scroll_px += item_track.height();
  m_scrollbar->content_size(m_max_scroll_px);
  m_just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::insert_track(db::track_info ti) { insert_track(m_items.size(), ti); }

void PanelTracklist::insert_item(const Item& item) { insert_item(m_items.size(), item); }

void PanelTracklist::insert_item(size_t index, const Item& item) {
  index = std::min(index, m_items.size());
  Item new_item = item;
  set_item_index(new_item, index);
  m_items.insert(m_items.begin() + index, new_item);
  for (size_t i = index; i < m_items.size(); i += 1) {
    set_item_index(m_items[i], i);
  }
  m_max_scroll_px += get_item_height(new_item);
  m_scrollbar->content_size(m_max_scroll_px);
  m_just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::set_item(size_t index, const Item& item) {
  if (index >= m_items.size()) { return; }
  i32 prev_height = get_item_height(m_items[index]);
  Item new_item = item;
  set_item_index(new_item, index);
  m_items[index] = new_item;
  m_max_scroll_px += (get_item_height(new_item) - prev_height);
  m_scrollbar->content_size(m_max_scroll_px);
  m_just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::set_track(size_t index, db::track_info ti) {
  if (index >= m_items.size()) { return; }
  i32 prev_height = get_item_height(m_items[index]);
  ItemTrack item_track;
  item_track.track_info = ti;
  item_track.item_index = index;
  m_items[index] = item_track;
  m_max_scroll_px += (item_track.height() - prev_height);
  m_scrollbar->content_size(m_max_scroll_px);
  m_just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::remove_item(size_t index) {
  if (index >= m_items.size()) { return; }
  i32 h = get_item_height(m_items[index]);
  m_items.erase(m_items.begin() + index);
  for (size_t i = index; i < m_items.size(); i += 1) {
    set_item_index(m_items[i], i);
  }
  m_max_scroll_px -= h;
  m_scrollbar->content_size(m_max_scroll_px);
  m_just_recreated = true;
  m_selection.clear();
  rebuild_lookup();
}

void PanelTracklist::clear() {
  m_collection_id = std::nullopt;
  m_items.clear();
  for (auto&& widget : m_items_container->children()) {
    widget->marked_for_deletion(true);
  }
  m_max_scroll_px = 0;
  m_just_recreated = true;
  m_selection.clear();
  m_scrollbar->content_size(0);
  rebuild_lookup();
}

void PanelTracklist::clear_selection() {
  m_selection.clear();
  m_selection_modified = true;
}

void PanelTracklist::insert_to_selection_index(size_t index) {
  if (index < m_items.size()) {
    if (m_selection.insert(index)) { m_selection_modified = true; }
  }
}

void PanelTracklist::insert_to_selection_track(db::track_info track_info) {
  auto idx = find_item_track(track_info);
  if (idx) {
    if (m_selection.insert(*idx)) { m_selection_modified = true; }
  }
}

void PanelTracklist::scroll_to_playlist(size_t target_playlist_id, bool immediate) {
  i32 offset = 0;
  for (auto& item : m_items) {
    if (auto* h = std::get_if<ItemPlaylistHeader>(&item)) {
      if (h->playlist_id == target_playlist_id) { break; }
    }
    offset += get_item_height(item);
  }
  m_target_scroll_px = offset;
  m_target_scroll_px = std::clamp<double>(m_target_scroll_px, 0.0, std::max<double>(0.0, m_max_scroll_px - height()));
  m_scrollbar->scroll_offset(m_target_scroll_px);
  if (immediate) {
    m_scroll_px = m_target_scroll_px;
    m_scrollbar->skip_anim();
  }
}

void PanelTracklist::scroll_to_track(size_t playlist_id, size_t track_id, bool immediate) {
  i32 offset = 0;
  for (auto& item : m_items) {
    if (auto* t = std::get_if<ItemTrack>(&item)) {
      if (t->track_info.track_id == track_id && t->track_info.playlist_id == playlist_id) { break; }
    }
    offset += get_item_height(item);
  }
  m_target_scroll_px = std::clamp<double>(offset, 0.0, std::max<double>(0.0, m_max_scroll_px - height()));
  m_scrollbar->scroll_offset(m_target_scroll_px);
  if (immediate) {
    m_scroll_px = m_target_scroll_px;
    m_scrollbar->skip_anim();
  }
}

std::optional<size_t> PanelTracklist::find_item_track(db::track_info ti) const {
  auto it = m_track_lookup.find(ti);
  if (it != m_track_lookup.end()) { return it->second; }
  return std::nullopt;
}

float PanelTracklist::scroll_px() const { return m_target_scroll_px; }

PanelTracklist& PanelTracklist::scroll_px(float px, bool immediate) {
  m_scroll_px = px;
  m_target_scroll_px = px;
  m_scrollbar->scroll_offset(px);
  if (immediate) { m_scrollbar->skip_anim(); }
  return *this;
}

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
        i.widget->marked_for_deletion(true);
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
      using T = std::decay_t<decltype(i)>;
      if constexpr (T::is_selectable) {
        if (i.widget) { i.widget->is_selected(selected); }
      }
    },
    item);
}

void PanelTracklist::create_item_widget_if_null(Item& item) {
  if (get_item_widget(item)) { return; }

  zincgui::Widget* generic_widget = nullptr;

  if (auto* t = std::get_if<ItemTrack>(&item)) {
    auto* w = &m_items_container->add_child<WidgetTrack>();
    w->collection_id(t->track_info.collection_id)
      .playlist_id(t->track_info.playlist_id)
      .track_id(t->track_info.track_id)
      .track_number(t->track_info.index + 1)
      .item_index(t->item_index)
      .highlight_mode(m_track_highlight_mode);

    w->ignore_parents_layout(true).is_drawn(false);
    t->widget = w;
    generic_widget = w;
  } else if (auto* qt = std::get_if<ItemQueueTrack>(&item)) {
    auto* w = &m_items_container->add_child<WidgetTrack>();
    if (qt->queue_index < player::get_playing_queue().size()) {
      auto q_elem = player::get_playing_queue()[qt->queue_index];
      if (q_elem.is_track()) {
        w->collection_id(q_elem.track()->track_info.collection_id)
          .playlist_id(q_elem.track()->track_info.playlist_id)
          .track_id(q_elem.track()->track_info.track_id)
          .track_number(qt->queue_index + 1)
          .item_index(qt->item_index)
          .highlight_mode(m_track_highlight_mode);
      }
    }
    w->ignore_parents_layout(true).is_drawn(false);
    qt->widget = w;
    generic_widget = w;
  } else if (auto* qp = std::get_if<ItemQueuePlaylist>(&item)) {
    auto* w = &m_items_container->add_child<WidgetQueuePlaylistCollection>();
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
    w->ignore_parents_layout(true).is_drawn(false);
    qp->widget = w;
    generic_widget = w;
  } else if (auto* qc = std::get_if<ItemQueueCollection>(&item)) {
    auto* w = &m_items_container->add_child<WidgetQueuePlaylistCollection>();
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
    w->ignore_parents_layout(true).is_drawn(false);
    qc->widget = w;
    generic_widget = w;
  } else if (auto* h = std::get_if<ItemPlaylistHeader>(&item)) {
    auto* w = &m_items_container->add_child<WidgetPlaylistHeader>(h->collection_id, h->playlist_id);
    w->ignore_parents_layout(true).is_drawn(false);
    h->widget = w;
    w->button_sort()->on_press([this, collection_id = h->collection_id, playlist_id = h->playlist_id, w]() -> void {
      if (m_on_playlist_sort_button_pressed) {
        m_on_playlist_sort_button_pressed(collection_id, playlist_id, w->button_sort());
      }
    });
    w->button_more()->on_press([this, collection_id = h->collection_id, playlist_id = h->playlist_id, w]() -> void {
      if (m_on_playlist_more_options_invoked) {
        m_on_playlist_more_options_invoked(collection_id, playlist_id, w->button_more());
      }
    });
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
        m_selection_modified = true;
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
              if (is_item_selectable(m_items[j]) && m_selection.insert(j)) { m_selection_modified = true; }
            }
          } else {
            for (size_t j = *index_last; j <= item_index; j += 1) {
              if (is_item_selectable(m_items[j]) && m_selection.insert(j)) { m_selection_modified = true; }
            }
          }
        }
      } else if (m_on_item_lmb) {
        m_on_item_lmb(item_index);
        if (m_selection.clear()) { m_selection_modified = true; }
      }
    };

    auto handle_press_rmb = [item_index, this, generic_widget]() -> void {
      if (!m_selection.empty()) {
        if (m_selection.has(item_index) && m_on_selection_rmb) {
          m_on_selection_rmb(item_index);
        } else if (m_on_item_rmb) {
          clear_selection();
          m_on_item_rmb(item_index);
        }
      } else if (m_on_item_rmb) {
        m_on_item_rmb(item_index);
      }
    };

    if (auto* w = dynamic_cast<WidgetTrack*>(generic_widget)) {
      w->on_press(handle_press);
      w->on_press_rmb(handle_press_rmb);
    } else if (auto* w2 = dynamic_cast<WidgetQueuePlaylistCollection*>(generic_widget)) {
      w2->on_press(handle_press);
      w2->on_press_rmb(handle_press_rmb);
    }
  }
}

void PanelTracklist::rebuild_lookup() {
  m_track_lookup.clear();
  for (size_t i = 0; i < m_items.size(); ++i) {
    if (auto* t = std::get_if<ItemTrack>(&m_items[i])) { m_track_lookup[t->track_info] = i; }
  }
}
