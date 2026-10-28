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
    Notification(zincgui::Root& ui_, bool error = false);

    void update() override;

    float y_lerped{};
    i32 timer{};
    bool dismissed = false;
    zincgui::Label* label{};
};

class InterfaceNotifications : public zincgui::Widget {
  public:
    InterfaceNotifications(zincgui::Root& ui_);

    void push(std::string_view);
    void push_error(std::string_view);
    Notification* push_persistent(std::string_view);
    void dismiss(Notification*);
    void dismiss_after(Notification*, i32);

    void update() override;

  private:
    std::vector<Notification*> m_notifications;
};
