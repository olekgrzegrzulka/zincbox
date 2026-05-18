#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "common/types.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/widget.hpp"

class PopupController;
class Label;

struct popover_descriptor {
    std::string id;
    vec2i at;
    i32 distance;
    std::vector<std::string> button_labels;
    std::vector<std::function<void()>> button_actions;
    bool show_arrow = true;
};

class Popup : public Sprite {
  public:
    Popup(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

  public:
    [[deprecated]] std::function<void(Popup*)> on_confirm{};
    [[deprecated]] std::function<void(Popup*)> on_cancel{};

    void close() {
      if (on_close) { on_close(this); }
    }

  protected:
    PopupController& controller;
    std::function<void(Popup*)> on_close{};
};

class [[deprecated]] PopupOld : public Popup {
  public:
    PopupOld(UI& ui_, PopupController& controller_);

  public:
    Label& title;
    Widget& content;
};

struct [[deprecated]] popup_descriptor {
    std::string id;
    std::u32string_view title;
    std::optional<std::u32string_view> content;
    std::vector<std::u32string> button_labels;
    std::vector<std::function<void(PopupOld*)>> button_actions;
};

class Popover : public Sprite {
  public:
    Popover(UI& ui_);
    void update() override;
    void draw() override;

  protected:
    vec2i popover_pos;
};
