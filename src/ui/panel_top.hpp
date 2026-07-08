#pragma once
#include <functional>
#include <optional>
#include "tab_bar.hpp"
#include "ui_generic/sprite.hpp"

class PanelTop : public Sprite {
  public:
    PanelTop(UI& ui_);
    void update() override;
    void input() override;
    void recreate(std::optional<size_t> selected_collection_id);
    void select(size_t selected_collection_id);

  public:
    std::function<void(Widget*)> on_settings_button_pressed{};
    std::function<void(Widget*)> on_add_collection_button_pressed{};
    std::function<void(size_t collection_id)> on_collection_opened{};
    std::function<void()> on_queue_view_opened{};
    std::function<void(Tab*)> on_queue_rmb{};
    std::function<void(size_t collection_id, Widget*)> on_show_collection_actions_popover{};
    TabBar* get_tab_bar() { return tab_bar; }
    const Tab* get_queue_tab() const;

  protected:
    TabBar* tab_bar{};
    Button* button_left{};
    Button* button_right{};
    Button* button_settings{};
};
