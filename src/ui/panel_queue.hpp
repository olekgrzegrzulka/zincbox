#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include "common/signal.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

class WidgetTrack;

class PanelQueue final : public zincgui::Widget {
  public:
    PanelQueue(const PanelQueue&) = delete;
    PanelQueue(PanelQueue&&) = delete;
    PanelQueue& operator=(const PanelQueue&) = delete;
    PanelQueue& operator=(PanelQueue&&) = delete;
    PanelQueue(zincgui::Root& ui_);
    ~PanelQueue() override;

    using Widget::event;
    void update() override;

    PanelQueue& show();
    PanelQueue& hide();
    void recreate();
    void clear();
    void clear_selection();
    void on_queue_appended_to_back();
    void on_queue_changed();
    void on_queue_changed_at(size_t at);
    void insert_to_selection(size_t i);

    [[nodiscard]] const PanelTracklistSelection& selection() const;
    [[nodiscard]] std::span<const PanelTracklist::Item> items() const;
    [[nodiscard]] const PanelTracklist::Item& item(size_t i) const;

    PanelQueue& insert_cursor_index(std::optional<size_t> value);

    PanelQueue& insert_cursor_pos(PanelTracklist::InsertCursorPos value);

    PanelQueue& is_dragged(bool value);

    PanelQueue& on_item_lmb(const std::function<void(size_t item_index)>& fn);
    PanelQueue& on_item_rmb(const std::function<void(size_t item_index)>& fn);
    PanelQueue& on_selection_rmb(const std::function<void(size_t item_index)>& fn);

    [[nodiscard]] PanelTracklist* panel_tracklist() noexcept { return m_panel_tracklist; }
    [[nodiscard]] const PanelTracklist* panel_tracklist() const noexcept { return m_panel_tracklist; }

  private:
    PanelTracklist* m_panel_tracklist{};
    Signal<>::slot_key m_slot_on_queue_changed;
    Signal<>::slot_key m_slot_on_queue_changed_at;
};
