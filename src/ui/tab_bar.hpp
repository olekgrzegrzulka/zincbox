#pragma once
#include <functional>
#include <span>
#include <string>
#include <vector>
#include <stddef.h>
#include "common/types.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

class Tab;

struct tab_info {
    i32 id{};
    bool is_draggable{};
    std::string label{};
    i32 padding = 0;
    std::function<void()> on_open{};
    std::function<void(Tab*)> on_right_click{};
};

class Tab : public zincgui::Button {
  public:
    Tab(zincgui::Root& ui_, const tab_info& info);

    void set_texture_active();
    void set_texture_inactive();
    void move_smooth(i32 to);
    void move(i32 to);
    void update() override;
    void event(zincgui::Input::InputEventMouseButton& ev) override;

  public:
    bool active = false;
    bool is_draggable = true;
    i32 padding = 0;
    size_t index = 0;
    i32 id = 0;
    std::function<void(i32)> on_drag_start{};
    std::function<void(Tab*)> on_active{};
    std::function<void(Tab*)> on_right_click{};

  protected:
    i32 x_old = 0;
    i32 x_new = 0;
    float t = 0.0f;
    bool just_added = true;
};

class TabBar : public zincgui::Widget {
  public:
    TabBar(zincgui::Root& ui_);
    virtual void event(zincgui::Input::InputEventMouseScroll&) override;

    void add_tab(const tab_info& info, bool select = false);
    void add_tab(const tab_info& info, size_t at, bool select = false);
    void select_tab(i32 id);
    void unselect_all_tabs();
    void close_tab(i32 id);
    void close_all_tabs();
    void update() override;
    void update_tab_textures(i32 id);
    void skip_anim();
    const std::vector<Tab*>& get_tabs() const { return tabs; }
    void sort_tabs_by_label(std::span<const std::string>);
    const Tab* get_selected_tab() const { return tab_valid(selected_tab_index) ? tabs[selected_tab_index] : nullptr; }
    const Tab* get_tab_by_label(const std::string& label) const;
    zincgui::Button* get_button_add() { return button_add; }
    i32 get_tab_container_width();
    double get_scroll_px() { return scroll_px; }
    double get_max_scroll_px();
    void scroll(double);

  protected:
    void position_tabs(bool smooth);
    void on_tab_drag_start(i32 id);
    bool tab_valid(size_t index) const;
    bool swap_tabs(size_t index_a, size_t index_b);

  public:
    std::function<void(i32)> on_tab_pressed{};
    std::function<void()> on_add_tab_button_pressed{};

  protected:
    zincgui::Widget* tab_container{};
    zincgui::Button* button_add{};
    zincgui::Button* button_left{};
    zincgui::Button* button_right{};
    i32 selected_tab_index = -1;
    std::vector<Tab*> tabs;
    i32 dragged_tab_index = -1;
    i32 drag_start_mouse_pos = 0;
    i32 drag_start_tab_pos = 0;
    double scroll_px = 0.0;
    double target_scroll_px = 0.0;
};
