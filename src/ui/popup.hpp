#pragma once
#include <functional>
#include <string>
#include <tuple>
#include <vector>
#include "common/types.hpp"
#include "ui_generic/sprite.hpp"

class PopupController;
class UI;

struct popover_descriptor {
    std::string id;
    std::u32string title;
    vec2i at;
    i32 distance;
    std::vector<std::tuple<std::u32string, std::function<void()>, std::string>> buttons;
    bool show_arrow = true;
};

class Popup : public Sprite {
  public:
    Popup(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

  public:
    void close() {
      if (on_close) { on_close(this); }
    }

    void update() override;
    void event(Input::InputEventMouseButton&) override;

  protected:
    PopupController& controller;
    std::function<void(Popup*)> on_close{};
    bool is_dragged = false;
    vec2i drag_start_pos{};
    vec2i drag_start_mouse_pos{};
};

class Popover : public Sprite {
  public:
    Popover(UI& ui_, bool arrow_on_top);
    void update() override;
    void draw() override;

  public:
    Sprite* arrow{};
    i32 arrow_offset = 0;
};
