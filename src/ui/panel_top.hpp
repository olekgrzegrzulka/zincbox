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

    [[nodiscard]] const Tab* queue_tab() const;

    [[nodiscard]] TabBar* tab_bar() noexcept { return m_tab_bar; }
    [[nodiscard]] const TabBar* tab_bar() const noexcept { return m_tab_bar; }

    [[nodiscard]] zincgui::Button* button_hamburger() noexcept { return m_button_hamburger; }
    [[nodiscard]] const zincgui::Button* button_hamburger() const noexcept { return m_button_hamburger; }
    [[nodiscard]] zincgui::Button* button_decor_minimize() noexcept { return m_button_decor_minimize; }
    [[nodiscard]] const zincgui::Button* button_decor_minimize() const noexcept { return m_button_decor_minimize; }
    [[nodiscard]] zincgui::Button* button_decor_maximize() noexcept { return m_button_decor_maximize; }
    [[nodiscard]] const zincgui::Button* button_decor_maximize() const noexcept { return m_button_decor_maximize; }
    [[nodiscard]] zincgui::Button* button_decor_close() noexcept { return m_button_decor_close; }
    [[nodiscard]] const zincgui::Button* button_decor_close() const noexcept { return m_button_decor_close; }
    [[nodiscard]] zincgui::Widget* container_drag_region() noexcept { return m_container_drag_region; }
    [[nodiscard]] const zincgui::Widget* container_drag_region() const noexcept { return m_container_drag_region; }

    PanelTop& on_hamburger_button_pressed(std::function<void(zincgui::Widget*)> cb) {
      m_on_hamburger_button_pressed = std::move(cb);
      return *this;
    }
    PanelTop& on_minimize_button_pressed(std::function<void()> cb) {
      m_on_minimize_button_pressed = std::move(cb);
      return *this;
    }
    PanelTop& on_maximize_button_pressed(std::function<void()> cb) {
      m_on_maximize_button_pressed = std::move(cb);
      return *this;
    }
    PanelTop& on_close_button_pressed(std::function<void()> cb) {
      m_on_close_button_pressed = std::move(cb);
      return *this;
    }
    PanelTop& on_add_collection_button_pressed(std::function<void(zincgui::Widget*)> cb) {
      m_on_add_collection_button_pressed = std::move(cb);
      return *this;
    }
    PanelTop& on_collection_opened(std::function<void(size_t collection_id)> cb) {
      m_on_collection_opened = std::move(cb);
      return *this;
    }
    PanelTop& on_queue_view_opened(std::function<void()> cb) {
      m_on_queue_view_opened = std::move(cb);
      return *this;
    }
    PanelTop& on_queue_rmb(std::function<void(Tab*)> cb) {
      m_on_queue_rmb = std::move(cb);
      return *this;
    }
    PanelTop& on_show_collection_actions_popover(std::function<void(size_t collection_id, Widget*)> cb) {
      m_on_show_collection_actions_popover = std::move(cb);
      return *this;
    }

  private:
    i32 m_width_old = -1;
    i32 m_height_old = -1;
    zincgui::Widget* m_container_tabbar{};
    zincgui::Widget* m_container_drag_region{};
    zincgui::Widget* m_container_buttons{};
    TabBar* m_tab_bar{};
    zincgui::Button* m_button_hamburger{};
    zincgui::Button* m_button_decor_minimize{};
    zincgui::Button* m_button_decor_maximize{};
    zincgui::Button* m_button_decor_close{};

    std::function<void(zincgui::Widget*)> m_on_hamburger_button_pressed{};
    std::function<void()> m_on_minimize_button_pressed{};
    std::function<void()> m_on_maximize_button_pressed{};
    std::function<void()> m_on_close_button_pressed{};
    std::function<void(zincgui::Widget*)> m_on_add_collection_button_pressed{};
    std::function<void(size_t collection_id)> m_on_collection_opened{};
    std::function<void()> m_on_queue_view_opened{};
    std::function<void(Tab*)> m_on_queue_rmb{};
    std::function<void(size_t collection_id, Widget*)> m_on_show_collection_actions_popover{};
};
