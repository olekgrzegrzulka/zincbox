#pragma once
#include <string_view>
#include <vector>
#include "ui/theme.hpp"
#include "ui/zincgui/button.hpp"

namespace zincgui {
  class Label;
  class Root;
  class Widget;
}; // namespace zincgui

class Notification final : public zincgui::Button {
  public:
    Notification(zincgui::Root& ui_, std::string_view message, bool error = false);

    void update() override;

    [[nodiscard]] float y_lerped() const noexcept { return m_y_lerped; }
    Notification& y_lerped(float v) noexcept {
      m_y_lerped = v;
      return *this;
    }

    [[nodiscard]] i32 timer() const noexcept { return m_timer; }
    Notification& timer(i32 v) noexcept {
      m_timer = v;
      return *this;
    }

    [[nodiscard]] bool dismissed() const noexcept { return m_dismissed; }
    Notification& dismissed(bool v) noexcept {
      m_dismissed = v;
      return *this;
    }

  private:
    float m_y_lerped{};
    i32 m_timer{};
    bool m_dismissed = false;
};

class InterfaceNotifications : public zincgui::Widget {
  public:
    InterfaceNotifications(zincgui::Root& ui_);

    void update() override;

    void push(std::string_view message);
    void push_error(std::string_view message);
    Notification* push_persistent(std::string_view message);
    void dismiss(Notification* w);
    void dismiss_after(Notification* w, i32 frames);

  private:
    std::vector<Notification*> m_notifications;
};
