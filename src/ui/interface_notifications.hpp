#pragma once
#include <string_view>
#include <vector>
#include "ui/theme.hpp"
#include "ui/zincgui/sprite.hpp"

namespace zincgui {
  class Label;
  class Root;
  class Widget;
}; // namespace zincgui

class Notification : public zincgui::Sprite {
  public:
    Notification(zincgui::Root& ui_);

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

    Notification* push_persistent(std::string_view);
    void dismiss(Notification*);
    void dismiss_after(Notification*, i32);

    void update() override;

  protected:
    std::vector<Notification*> notifications;
};
