#pragma once
#include <cstddef>
#include <functional>
#include <vector>
#include <glm/common.hpp>
#include "common/input.hpp"
#include "common/signal.hpp"
#include "common/types.hpp"
#include "ui_generic/panel.hpp"

class ScrollBar;
class WidgetTrack;

class PanelQueue : public Panel {
  public:
    PanelQueue(UI& ui_);
    ~PanelQueue() override;

    void draw() override;
    void on_view_changed();
    void on_queue_appended_to_back();
    void on_queue_changed();
    void on_queue_changed_at(size_t);
    void update() override;
    using Panel::handle_event;
    void handle_event(Input::InputEventMouseScroll&) override;
    void clear();

  public:
    std::function<void(size_t queue_index, Widget* widget)> on_queue_element_lmb{};
    std::function<void(size_t queue_index, Widget* widget)> on_queue_element_rmb{};

  protected:
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    i32 old_width{};
    i32 max_scroll_px{};
    ScrollBar* scrollbar{};
    std::vector<std::pair<i32, size_t>> album_scroll_px;
    std::vector<WidgetTrack*> queue_tracks;

    Signal<>::slot_key slot_on_track_changed;
    Signal<>::slot_key slot_on_queue_changed;
};
