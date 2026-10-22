#pragma once
#include <functional>
#include <optional>
#include <stddef.h>
#include "ui/zincgui/color_rect.hpp"

namespace zincgui {
  class Button;
  class Root;
  class Widget;
} // namespace zincgui

class Tab;
class TabBar;

class PanelTop final : public zincgui::ColorRect {
  public:
    PanelTop(const PanelTop&) = delete;
    PanelTop(PanelTop&&) = delete;
    PanelTop& operator=(const PanelTop&) = delete;
    PanelTop& operator=(PanelTop&&) = delete;
    PanelTop(zincgui::Root& ui_);
    ~PanelTop() override = default;
    void update() override;
    void recreate(std::optional<size_t> selected_collection_id);
    void select(size_t selected_collection_id);
    bool can_drag_window();

  public:
    std::function<void(zincgui::Widget*)> on_hamburger_button_pressed{};
    std::function<void()> on_minimize_button_pressed{};
    std::function<void()> on_maximize_button_pressed{};
    std::function<void()> on_close_button_pressed{};
    std::function<void(zincgui::Widget*)> on_add_collection_button_pressed{};
    std::function<void(size_t collection_id)> on_collection_opened{};
    std::function<void()> on_queue_view_opened{};
    std::function<void(Tab*)> on_queue_rmb{};
    std::function<void(size_t collection_id, Widget*)> on_show_collection_actions_popover{};
    TabBar* get_tab_bar() { return m_tab_bar; }
    const Tab* get_queue_tab() const;

  private:
    zincgui::Widget* m_container_tabbar{};
    zincgui::Widget* m_container_drag_region{};
    zincgui::Widget* m_container_buttons{};
    TabBar* m_tab_bar{};
    zincgui::Button* m_button_hamburger{};
    zincgui::Button* m_button_decor_minimize{};
    zincgui::Button* m_button_decor_maximize{};
    zincgui::Button* m_button_decor_close{};
};
