#pragma once
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "common/types.hpp"
#include "ui/zincgui/sprite.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

class PopupController;

struct popover_descriptor {
    std::string id;
    std::string title;
    vec2i at;
    i32 distance;
    std::vector<std::tuple<std::string, std::function<void()>, std::string>> buttons;
    bool show_arrow = true;
};

class Popup : public zincgui::Sprite {
  public:
    Popup(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    void update() override;
    void event(zincgui::Input::InputEventMouseButton& e) override;
    void event(zincgui::Input::InputEventKey& e) override;

    void close() {
      if (m_on_close) { m_on_close(this); }
    }

    [[nodiscard]] i32 max_content_width() const;
    [[nodiscard]] i32 max_content_height() const;

    [[nodiscard]] PopupController& controller() noexcept { return m_controller; }
    [[nodiscard]] const PopupController& controller() const noexcept { return m_controller; }

    [[nodiscard]] bool is_dragged() const noexcept { return m_is_dragged; }
    Popup& is_dragged(bool v) noexcept {
      m_is_dragged = v;
      return *this;
    }

    Popup& on_close(std::function<void(Popup*)> fn) {
      m_on_close = std::move(fn);
      return *this;
    }

  private:
    PopupController& m_controller;
    std::function<void(Popup*)> m_on_close{};
    bool m_is_dragged = false;
    vec2i m_drag_start_pos{};
    vec2i m_drag_start_mouse_pos{};
};

class Popover : public zincgui::Sprite {
  public:
    Popover(zincgui::Root& ui_, bool arrow_on_top);

    void update() override;

    [[nodiscard]] Sprite* arrow() noexcept { return m_arrow; }
    [[nodiscard]] const Sprite* arrow() const noexcept { return m_arrow; }

    [[nodiscard]] i32 arrow_offset() const noexcept { return m_arrow_offset; }
    Popover& arrow_offset(i32 v) noexcept {
      m_arrow_offset = v;
      return *this;
    }

  private:
    Sprite* m_arrow{};
    i32 m_arrow_offset = 0;
};
