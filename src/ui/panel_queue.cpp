#include "panel_queue.hpp"
#include <utility>
#include <vector>
#include "common/logger.hpp"
#include "core/player/player.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/widget.hpp"
#include "widget_track.hpp"

using namespace zincgui;

PanelQueue::PanelQueue(Root& ui_) : Widget(ui_) {
  clip_children(true);

  m_panel_tracklist = &add_child<PanelTracklist>();
  m_panel_tracklist->track_highlight_mode(WidgetTrack::TrackHighlightMode::QUEUE_INDEX);

  m_slot_on_queue_changed = player::signal_on_queue_changed.connect([this](bool track_appended_to_back) -> void {
    if (track_appended_to_back) {
      on_queue_appended_to_back();
    } else {
      on_queue_changed();
    }
  });

  m_slot_on_queue_changed_at =
    player::signal_on_queue_changed_at.connect([this](size_t at) -> void { on_queue_changed_at(at); });
}

PanelQueue::~PanelQueue() {
  player::signal_on_queue_changed.disconnect(m_slot_on_queue_changed);
  player::signal_on_queue_changed_at.disconnect(m_slot_on_queue_changed_at);
}

void PanelQueue::update() {
  m_panel_tracklist->size(width(), height());
  Widget::update();
}

PanelQueue& PanelQueue::show() {
  is_drawn(true)
    .is_updated(true);
  recreate();
  m_panel_tracklist->show();
  return *this;
}

PanelQueue& PanelQueue::hide() {
  is_drawn(false)
    .is_updated(false);
  m_panel_tracklist->hide();
  return *this;
}

void PanelQueue::recreate() { on_queue_changed(); }

void PanelQueue::clear() { m_panel_tracklist->clear(); }

void PanelQueue::clear_selection() { m_panel_tracklist->clear_selection(); }

void PanelQueue::on_queue_appended_to_back() {
  if (player::get_playing_queue().empty()) { return; }
  size_t i = player::get_playing_queue().size() - 1;
  auto& elem = player::get_playing_queue().back();

  if (elem.is_track()) {
    PanelTracklist::ItemQueueTrack item_track;
    item_track.queue_index = i;
    m_panel_tracklist->insert_item(item_track);
  } else if (elem.is_playlist()) {
    PanelTracklist::ItemQueuePlaylist item_playlist;
    item_playlist.queue_index = i;
    m_panel_tracklist->insert_item(item_playlist);
  } else if (elem.is_collection()) {
    PanelTracklist::ItemQueueCollection item_collection;
    item_collection.queue_index = i;
    m_panel_tracklist->insert_item(item_collection);
  }
}

void PanelQueue::on_queue_changed() {
  if (player::get_playing_queue().empty()) {
    m_panel_tracklist->clear();
    return;
  }
  std::vector<PanelTracklist::Item> queue_items;
  queue_items.reserve(player::get_playing_queue().size());
  for (size_t i = 0; i < player::get_playing_queue().size(); i += 1) {
    auto& elem = player::get_playing_queue()[i];
    if (elem.is_track()) {
      PanelTracklist::ItemQueueTrack item_track;
      item_track.queue_index = i;
      queue_items.emplace_back(item_track);
    } else if (elem.is_playlist()) {
      PanelTracklist::ItemQueuePlaylist item_playlist;
      item_playlist.queue_index = i;
      queue_items.emplace_back(item_playlist);
    } else if (elem.is_collection()) {
      PanelTracklist::ItemQueueCollection item_collection;
      item_collection.queue_index = i;
      queue_items.emplace_back(item_collection);
    }
  }
  m_panel_tracklist->recreate(queue_items);
}

void PanelQueue::on_queue_changed_at(size_t queue_i) {
  if (player::get_playing_queue().size() <= queue_i) { return; }
  auto& elem = player::get_playing_queue()[queue_i];

  if (elem.is_track()) {
    PanelTracklist::ItemQueueTrack item_track;
    item_track.queue_index = queue_i;
    m_panel_tracklist->set_item(queue_i, item_track);
  } else if (elem.is_playlist()) {
    PanelTracklist::ItemQueuePlaylist item_playlist;
    item_playlist.queue_index = queue_i;
    m_panel_tracklist->set_item(queue_i, item_playlist);
  } else if (elem.is_collection()) {
    PanelTracklist::ItemQueueCollection item_collection;
    item_collection.queue_index = queue_i;
    m_panel_tracklist->set_item(queue_i, item_collection);
  }
}

void PanelQueue::insert_to_selection(size_t i) {
  if (i >= player::get_playing_queue().size()) { return; }
  m_panel_tracklist->insert_to_selection_index(i);
}

const PanelTracklistSelection& PanelQueue::selection() const { return m_panel_tracklist->selection(); }

std::span<const PanelTracklist::Item> PanelQueue::items() const { return m_panel_tracklist->items(); }

const PanelTracklist::Item& PanelQueue::item(size_t i) const { return m_panel_tracklist->item(i); }

PanelQueue& PanelQueue::insert_cursor_index(std::optional<size_t> value) {
  m_panel_tracklist->insert_cursor_index(value);
  return *this;
}

PanelQueue& PanelQueue::insert_cursor_pos(PanelTracklist::InsertCursorPos value) {
  m_panel_tracklist->insert_cursor_pos(value);
  return *this;
}

PanelQueue& PanelQueue::is_dragged(bool value) {
  m_panel_tracklist->is_dragged(value);
  return *this;
}

PanelQueue& PanelQueue::on_item_lmb(const std::function<void(size_t)>& fn) {
  if (fn) { m_panel_tracklist->on_item_lmb(fn); }
  return *this;
}

PanelQueue& PanelQueue::on_item_rmb(const std::function<void(size_t)>& fn) {
  if (fn) { m_panel_tracklist->on_item_rmb(fn); }
  return *this;
}

PanelQueue& PanelQueue::on_selection_rmb(const std::function<void(size_t)>& fn) {
  if (fn) { m_panel_tracklist->on_selection_rmb(fn); }
  return *this;
}
