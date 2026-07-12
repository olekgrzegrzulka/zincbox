#include "panel_queue.hpp"
#include <vector>
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "ui/panel_tracks.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"
#include "widget_track.hpp"

PanelQueue::PanelQueue(UI& ui_) : Widget(ui_) {
  set_clip_children(true);

  panel_tracks = &add_child<PanelTracks>();
  panel_tracks->set_track_highlight_mode(WidgetTrack::TrackHighlightMode::QUEUE_INDEX);

  slot_on_queue_changed = player::signal_on_queue_changed.connect([this](bool track_appended_to_back) -> void {
    if (track_appended_to_back) {
      this->on_queue_appended_to_back();
    } else {
      this->on_queue_changed();
    }
  });
}

PanelQueue::~PanelQueue() { player::signal_on_queue_changed.disconnect(slot_on_queue_changed); }

void PanelQueue::draw() { Widget::draw(); }

void PanelQueue::on_queue_appended_to_back() {
  if (player::get_playing_queue().empty()) { return; }
  db::track_info ti = player::get_playing_queue().back();
  ti.index = player::get_playing_queue().size() - 1;
  panel_tracks->insert_track(ti);
}

void PanelQueue::on_queue_changed_at(size_t queue_i) {
  if (player::get_playing_queue().size() <= queue_i) { return; }
  db::track_info ti = player::get_playing_queue()[queue_i];
  ti.index = queue_i;
  panel_tracks->set_track(queue_i, ti);
}

void PanelQueue::on_queue_changed() {
  if (player::get_playing_queue().empty()) {
    panel_tracks->clear();
    return;
  }
  std::vector<db::track_info> queue;
  queue.reserve(player::get_playing_queue().size());
  for (size_t i = 0; i < player::get_playing_queue().size(); i += 1) {
    db::track_info ti = player::get_playing_queue()[i];
    ti.index = i;
    queue.emplace_back(ti);
  }
  panel_tracks->recreate(queue);
}

void PanelQueue::recreate() { on_queue_changed(); }

void PanelQueue::clear() { panel_tracks->clear(); }

void PanelQueue::update() {
  panel_tracks->set_size({width, height});
  Widget::update();
}

void PanelQueue::on_track_lmb(
  const std::function<void(db::track_info, size_t playlist_track_index, WidgetTrack*)>& fn) {
  if (fn) { panel_tracks->on_track_lmb = fn; }
}

void PanelQueue::on_track_rmb(
  const std::function<void(db::track_info, size_t playlist_track_index, WidgetTrack*)>& fn) {
  if (fn) { panel_tracks->on_track_rmb = fn; }
}

void PanelQueue::on_selection_rmb(const std::function<void(WidgetTrack*)>& fn) {
  if (fn) { panel_tracks->on_selection_rmb = fn; }
}
