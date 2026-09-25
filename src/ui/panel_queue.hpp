#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include "common/signal.hpp"
#include "core/musicdb/types.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

class WidgetTrack;

class PanelQueue : public zincgui::Widget {
    using Widget::event;

  public:
    PanelQueue(zincgui::Root& ui_);
    ~PanelQueue() override;
    void update() override;
    void draw() override;

    void show() {
      set_is_drawn(true);
      set_is_updated(true);
      recreate();
      input();
      update();
      panel_tracklist->show();
    }

    void hide() {
      set_is_drawn(false);
      set_is_updated(false);
      panel_tracklist->hide();
    }

    void recreate();
    void clear();
    const PanelTracklistSelection& selection() const { return panel_tracklist->selection(); }
    void clear_selection() { panel_tracklist->clear_selection(); }

    void set_insert_cursor_track_info(std::optional<db::track_info> value) {
      panel_tracklist->set_insert_cursor_track_info(value);
    }
    void set_insert_cursor_pos(PanelTracklist::InsertCursorPos value) { panel_tracklist->set_insert_cursor_pos(value); }

    void on_queue_appended_to_back();
    void on_queue_changed();
    void on_queue_changed_at(size_t);

    std::span<const PanelTracklist::Item> get_items() const { return panel_tracklist->get_items(); }

  public:
    void on_track_lmb(const std::function<void(db::track_info, WidgetTrack*)>&);
    void on_track_rmb(const std::function<void(db::track_info, WidgetTrack*)>&);
    void on_selection_rmb(const std::function<void(WidgetTrack*)>&);
    void set_is_dragged(bool);
    void insert_to_selection(size_t);

  protected:
    PanelTracklist* panel_tracklist{};
    Signal<>::slot_key slot_on_queue_changed;
};
