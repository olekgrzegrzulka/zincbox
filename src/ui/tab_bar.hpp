#pragma once
#include <functional>
#include <span>
#include <string>
#include <utility>
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
    std::string label;
    i32 padding = 0;
    std::function<void()> on_open;
    std::function<void(Tab*)> on_right_click;
};

class Tab final : public zincgui::Button {
  public:
    Tab(zincgui::Root& ui_, const tab_info& info);

    void update() override;
    void event(zincgui::Input::InputEventMouseButton& ev) override;
    void on_state_changed(zincgui::ButtonState /* prev_state */) override;

    Tab& texture_active();
    Tab& texture_inactive();
    void move_smooth(i32 to);
    void move(i32 to);

    [[nodiscard]] bool is_draggable() const noexcept { return m_is_draggable; }
    Tab& is_draggable(bool v) noexcept {
      m_is_draggable = v;
      return *this;
    }

    [[nodiscard]] i32 padding() const noexcept { return m_padding; }
    Tab& padding(i32 v) noexcept {
      m_padding = v;
      return *this;
    }

    [[nodiscard]] size_t index() const noexcept { return m_index; }
    Tab& index(size_t v) noexcept {
      m_index = v;
      return *this;
    }

    [[nodiscard]] i32 id() const noexcept { return m_id; }
    Tab& id(i32 v) noexcept {
      m_id = v;
      return *this;
    }

    [[nodiscard]] bool active() const noexcept { return m_active; }
    Tab& active(bool a);

    Tab& on_drag_start(std::function<void(i32)> fn) {
      m_on_drag_start = std::move(fn);
      return *this;
    }

    Tab& on_active(std::function<void(Tab*)> fn) {
      m_on_active = std::move(fn);
      return *this;
    }

    Tab& on_right_click(std::function<void(Tab*)> fn) {
      m_on_right_click = std::move(fn);
      return *this;
    }

    void trigger_drag_start(i32 idx) {
      if (m_on_drag_start) { m_on_drag_start(idx); }
    }
    void trigger_active() {
      if (m_on_active) { m_on_active(this); }
    }
    void trigger_right_click() {
      if (m_on_right_click) { m_on_right_click(this); }
    }

  private:
    bool m_is_draggable = true;
    i32 m_padding = 0;
    size_t m_index = 0;
    i32 m_id = 0;
    std::function<void(i32)> m_on_drag_start;
    std::function<void(Tab*)> m_on_active;
    std::function<void(Tab*)> m_on_right_click;
    bool m_active = false;
    i32 m_x_old = 0;
    i32 m_x_new = 0;
    float m_t = 0.0f;
    bool m_just_added = true;
};

class TabBar : public zincgui::Widget {
  public:
    TabBar(zincgui::Root& ui_);

    void update() override;
    void event(zincgui::Input::InputEventMouseScroll& ev) override;

    void add_tab(const tab_info& info, bool select = false);
    void add_tab(const tab_info& info, size_t at, bool select = false);
    void select_tab(i32 id);
    void unselect_all_tabs();
    void close_tab(i32 id);
    void close_all_tabs();
    void update_tab_textures(i32 id);
    void skip_anim();
    void sort_tabs_by_label(std::span<const std::string> labels);
    void scroll(double value);

    [[nodiscard]] const std::vector<Tab*>& tabs() const noexcept { return m_tabs; }
    [[nodiscard]] const Tab* selected_tab() const noexcept {
      return tab_valid(m_selected_tab_index) ? m_tabs[m_selected_tab_index] : nullptr;
    }
    [[nodiscard]] const Tab* tab_by_label(const std::string& label) const;
    [[nodiscard]] zincgui::Button* button_add() noexcept { return m_button_add; }
    [[nodiscard]] const zincgui::Button* button_add() const noexcept { return m_button_add; }
    [[nodiscard]] i32 tab_container_width();
    [[nodiscard]] double scroll_px() const noexcept { return m_scroll_px; }
    [[nodiscard]] double max_scroll_px();
    [[nodiscard]] i32 content_size() const noexcept { return m_content_size; }

    TabBar& on_tab_pressed(std::function<void(i32)> fn) {
      m_on_tab_pressed = std::move(fn);
      return *this;
    }

    TabBar& on_add_tab_button_pressed(std::function<void()> fn) {
      m_on_add_tab_button_pressed = std::move(fn);
      return *this;
    }

  protected:
    void position_tabs(bool smooth);
    void on_tab_drag_start(i32 id);
    bool tab_valid(size_t index) const;
    bool swap_tabs(size_t index_a, size_t index_b);

  private:
    std::function<void(i32)> m_on_tab_pressed{};
    std::function<void()> m_on_add_tab_button_pressed{};
    i32 m_content_size = 0;
    zincgui::Widget* m_tab_container{};
    zincgui::Button* m_button_add{};
    zincgui::Button* m_button_left{};
    zincgui::Button* m_button_right{};
    i32 m_selected_tab_index = -1;
    std::vector<Tab*> m_tabs;
    i32 m_dragged_tab_index = -1;
    i32 m_drag_start_mouse_pos = 0;
    i32 m_drag_start_tab_pos = 0;
    double m_scroll_px = 0.0;
    double m_target_scroll_px = 0.0;
};
