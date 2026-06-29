#pragma once
#include <functional>
#include <string>
#include <vector>
#include "common/input.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/widget.hpp"

class UI;
class Label;

class Tab : public Button {
  public:
    Tab(UI& ui_);

    void set_texture_active();
    void set_texture_inactive();
    void move_smooth(i32 to);
    void move(i32 to);
    void update() override;
    void event(Input::InputEventMouseButton& ev) override;

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

class TabBar : public Sprite {
  public:
    TabBar(UI& ui_);

    struct tab_info {
        i32 id{};
        bool is_draggable{};
        std::u32string label{};
        i32 padding = 0;
        std::function<void()> on_open{};
        std::function<void(Tab*)> on_right_click{};
    };

    void add_tab(const tab_info& info, bool select = false);
    void add_tab(const tab_info& info, size_t at, bool select = false);
    void open_tab(i32 id);
    void close_tab(i32 id);
    void close_all_tabs();
    void update() override;
    void update_tab_textures(i32 id);
    const std::vector<Tab*>& get_tabs() const { return tabs; }
    void sort_tabs_by_label(std::span<const std::u32string>);
    const Tab* get_selected_tab() const { return tab_valid(selected_tab_index) ? tabs[selected_tab_index] : nullptr; }

  protected:
    void on_tab_drag_start(i32 id);
    bool tab_valid(size_t index) const;
    bool swap_tabs(size_t index_a, size_t index_b);

  public:
    std::function<void(i32)> on_tab_pressed{};
    std::function<void()> on_add_tab_button_pressed{};

  protected:
    Widget* tab_container{};
    Button* button_add{};
    i32 selected_tab_index = -1;
    std::vector<Tab*> tabs;
    i32 dragged_tab_index = -1;
    i32 drag_start_mouse_pos = 0;
    i32 drag_start_tab_pos = 0;
};
