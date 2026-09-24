#pragma once
#include <functional>
#include <string>
#include <tuple>
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

  public:
    void close() {
      if (on_close) { on_close(this); }
    }

    void update() override;
    void event(zincgui::Input::InputEventMouseButton&) override;

    i32 get_max_content_width() const;
    i32 get_max_content_height() const;

  protected:
    PopupController& controller;
    std::function<void(Popup*)> on_close{};
    bool is_dragged = false;
    vec2i drag_start_pos{};
    vec2i drag_start_mouse_pos{};
};

class Popover : public zincgui::Sprite {
  public:
    Popover(zincgui::Root& ui_, bool arrow_on_top);
    void update() override;
    void draw() override;

  public:
    Sprite* arrow{};
    i32 arrow_offset = 0;
};
