#pragma once
#include <functional>
#include <optional>
#include <stddef.h>
#include "ui_generic/color_rect.hpp"

class Button;
class Tab;
class TabBar;
class UI;
class Widget;

class PanelTop : public ColorRect {
  public:
    PanelTop(UI& ui_);
    void update() override;
    void recreate(std::optional<size_t> selected_collection_id);
    void select(size_t selected_collection_id);
    bool can_drag_window();

  public:
    std::function<void(Widget*)> on_hamburger_button_pressed{};
    std::function<void()> on_minimize_button_pressed{};
    std::function<void()> on_maximize_button_pressed{};
    std::function<void()> on_close_button_pressed{};
    std::function<void(Widget*)> on_add_collection_button_pressed{};
    std::function<void(size_t collection_id)> on_collection_opened{};
    std::function<void()> on_queue_view_opened{};
    std::function<void(Tab*)> on_queue_rmb{};
    std::function<void(size_t collection_id, Widget*)> on_show_collection_actions_popover{};
    TabBar* get_tab_bar() { return tab_bar; }
    const Tab* get_queue_tab() const;

  protected:
    Widget* container_tabbar{};
    Widget* container_drag_region{};
    Widget* container_buttons{};
    TabBar* tab_bar{};
    Button* button_hamburger{};
    Button* button_decor_minimize{};
    Button* button_decor_maximize{};
    Button* button_decor_close{};
};
