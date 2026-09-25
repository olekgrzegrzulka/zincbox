#include "panel_queue.hpp"
#include <vector>
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
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
}

PanelQueue::~PanelQueue() { player::signal_on_queue_changed.disconnect(slot_on_queue_changed); }

void PanelQueue::draw() { Widget::draw(); }

void PanelQueue::on_queue_appended_to_back() {
  if (player::get_playing_queue().empty()) { return; }
  db::track_info ti = player::get_playing_queue().back();
  ti.index = player::get_playing_queue().size() - 1;
  panel_tracklist->insert_track(ti);
}

void PanelQueue::on_queue_changed_at(size_t queue_i) {
  if (player::get_playing_queue().size() <= queue_i) { return; }
  db::track_info ti = player::get_playing_queue()[queue_i];
  ti.index = queue_i;
  panel_tracklist->set_track(queue_i, ti);
}

void PanelQueue::on_queue_changed() {
  if (player::get_playing_queue().empty()) {
    panel_tracklist->clear();
    return;
  }
  std::vector<db::track_info> queue;
  queue.reserve(player::get_playing_queue().size());
  for (size_t i = 0; i < player::get_playing_queue().size(); i += 1) {
    db::track_info ti = player::get_playing_queue()[i];
    ti.index = i;
    queue.emplace_back(ti);
  }
  panel_tracklist->recreate(queue);
}

void PanelQueue::recreate() { on_queue_changed(); }

void PanelQueue::clear() { panel_tracklist->clear(); }

void PanelQueue::update() {
  panel_tracklist->set_size({width, height});
  Widget::update();
}

void PanelQueue::on_track_lmb(const std::function<void(db::track_info, WidgetTrack*)>& fn) {
  if (fn) { panel_tracklist->on_track_lmb = fn; }
}

void PanelQueue::on_track_rmb(const std::function<void(db::track_info, WidgetTrack*)>& fn) {
  if (fn) { panel_tracklist->on_track_rmb = fn; }
}

void PanelQueue::on_selection_rmb(const std::function<void(WidgetTrack*)>& fn) {
  if (fn) { panel_tracklist->on_selection_rmb = fn; }
}

void PanelQueue::set_is_dragged(bool value) { panel_tracklist->set_is_dragged(value); }

void PanelQueue::insert_to_selection(size_t i) {
  if (i >= player::get_playing_queue().size()) { return; }
  auto ti = player::get_playing_queue()[i];
  ti.index = i;
  panel_tracklist->insert_to_selection(ti);
}
