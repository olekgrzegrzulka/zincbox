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

    void set_insert_cursor_index(std::optional<size_t> value) { panel_tracklist->set_insert_cursor_index(value); }
    void set_insert_cursor_pos(PanelTracklist::InsertCursorPos value) { panel_tracklist->set_insert_cursor_pos(value); }

    void on_queue_appended_to_back();
    void on_queue_changed();
    void on_queue_changed_at(size_t);

    std::span<const PanelTracklist::Item> get_items() const { return panel_tracklist->get_items(); }

  public:
    void on_item_lmb(const std::function<void(size_t item_index)>&);
    void on_item_rmb(const std::function<void(size_t item_index)>&);
    void on_selection_rmb(const std::function<void(size_t item_index)>&);
    void set_is_dragged(bool);
    void insert_to_selection(size_t);
    const PanelTracklist::Item& get_item(size_t i) const { return panel_tracklist->get_item(i); }

  protected:
    PanelTracklist* panel_tracklist{};
    Signal<>::slot_key slot_on_queue_changed;
    Signal<>::slot_key slot_on_queue_changed_at;
};
