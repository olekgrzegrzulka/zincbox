#pragma once
#include <cstddef>
#include <functional>
#include "common/signal.hpp"
#include "core/musicdb/types.hpp"
#include "ui/panel_tracks.hpp"
#include "ui_generic/widget.hpp"

class UI;
class WidgetTrack;

class PanelQueue : public Widget {
    using Widget::event;

  public:
    PanelQueue(UI& ui_);
    ~PanelQueue() override;
    void update() override;
    void draw() override;

    void recreate();
    void clear();
    const PanelTracksSelection& selection() const { return panel_tracks->selection(); }
    void clear_selection() { panel_tracks->clear_selection(); }

    void on_queue_appended_to_back();
    void on_queue_changed();
    void on_queue_changed_at(size_t);

  public:
    void on_track_lmb(const std::function<void(db::track_info, size_t playlist_track_index, WidgetTrack*)>&);
    void on_track_rmb(const std::function<void(db::track_info, size_t playlist_track_index, WidgetTrack*)>&);
    void on_selection_rmb(const std::function<void(WidgetTrack*)>&);

  protected:
    PanelTracks* panel_tracks{};
    Signal<>::slot_key slot_on_queue_changed;
};
