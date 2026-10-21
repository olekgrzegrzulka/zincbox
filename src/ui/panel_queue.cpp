#include "panel_queue.hpp"
#include <vector>
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "widget_track.hpp"

using namespace zincgui;

PanelQueue::PanelQueue(Root& ui_) : Widget(ui_) {
  set_clip_children(true);

  panel_tracklist = &add_child<PanelTracklist>();
  panel_tracklist->set_track_highlight_mode(WidgetTrack::TrackHighlightMode::QUEUE_INDEX);

  slot_on_queue_changed = player::signal_on_queue_changed.connect([this](bool track_appended_to_back) -> void {
    if (track_appended_to_back) {
      this->on_queue_appended_to_back();
    } else {
      this->on_queue_changed();
    }
  });

  slot_on_queue_changed_at =
    player::signal_on_queue_changed_at.connect([this](size_t at) -> void { this->on_queue_changed_at(at); });
}

PanelQueue::~PanelQueue() {
  player::signal_on_queue_changed.disconnect(slot_on_queue_changed);
  player::signal_on_queue_changed_at.disconnect(slot_on_queue_changed_at);
}

void PanelQueue::draw() { Widget::draw(); }

void PanelQueue::on_queue_appended_to_back() {
  if (player::get_playing_queue().empty()) { return; }
  size_t i = player::get_playing_queue().size() - 1;
  auto& elem = player::get_playing_queue().back();

  if (elem.is_track()) {
    PanelTracklist::ItemQueueTrack item;
    item.queue_index = i;
    panel_tracklist->insert_item(item);
  } else if (elem.is_playlist()) {
    PanelTracklist::ItemQueuePlaylist item;
    item.queue_index = i;
    panel_tracklist->insert_item(item);
  } else if (elem.is_collection()) {
    PanelTracklist::ItemQueueCollection item;
    item.queue_index = i;
    panel_tracklist->insert_item(item);
  }
}

void PanelQueue::on_queue_changed_at(size_t queue_i) {
  if (player::get_playing_queue().size() <= queue_i) { return; }
  auto& elem = player::get_playing_queue()[queue_i];

  if (elem.is_track()) {
    PanelTracklist::ItemQueueTrack item;
    item.queue_index = queue_i;
    panel_tracklist->set_item(queue_i, item);
  } else if (elem.is_playlist()) {
    PanelTracklist::ItemQueuePlaylist item;
    item.queue_index = queue_i;
    panel_tracklist->set_item(queue_i, item);
  } else if (elem.is_collection()) {
    PanelTracklist::ItemQueueCollection item;
    item.queue_index = queue_i;
    panel_tracklist->set_item(queue_i, item);
  }
}

void PanelQueue::on_queue_changed() {
  if (player::get_playing_queue().empty()) {
    panel_tracklist->clear();
    return;
  }
  std::vector<PanelTracklist::Item> queue_items;
  queue_items.reserve(player::get_playing_queue().size());
  for (size_t i = 0; i < player::get_playing_queue().size(); i += 1) {
    auto& elem = player::get_playing_queue()[i];
    if (elem.is_track()) {
      PanelTracklist::ItemQueueTrack item;
      item.queue_index = i;
      queue_items.emplace_back(item);
    } else if (elem.is_playlist()) {
      PanelTracklist::ItemQueuePlaylist item;
      item.queue_index = i;
      queue_items.emplace_back(item);
    } else if (elem.is_collection()) {
      PanelTracklist::ItemQueueCollection item;
      item.queue_index = i;
      queue_items.emplace_back(item);
    }
  }
  panel_tracklist->recreate(queue_items);
}

void PanelQueue::recreate() { on_queue_changed(); }

void PanelQueue::clear() { panel_tracklist->clear(); }

void PanelQueue::update() {
  panel_tracklist->set_size({width, height});
  Widget::update();
}

void PanelQueue::on_item_lmb(const std::function<void(size_t)>& fn) {
  if (fn) { panel_tracklist->on_item_lmb = fn; }
}

void PanelQueue::on_item_rmb(const std::function<void(size_t)>& fn) {
  if (fn) { panel_tracklist->on_item_rmb = fn; }
}

void PanelQueue::on_selection_rmb(const std::function<void(size_t)>& fn) {
  if (fn) { panel_tracklist->on_selection_rmb = fn; }
}

void PanelQueue::set_is_dragged(bool value) { panel_tracklist->set_is_dragged(value); }

void PanelQueue::insert_to_selection(size_t i) {
  if (i >= player::get_playing_queue().size()) { return; }
  panel_tracklist->insert_to_selection_index(i);
}
