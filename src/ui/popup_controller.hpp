#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include "common/input.hpp"
#include "popup.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class Dimmer : public Sprite {
  public:
    Dimmer(UI& ui_) : Sprite(ui_, "dim") {}

    void set_is_active(bool active) {
      set_is_updated(active);
      if (!active) { prev_window_size = std::nullopt; }
    }

    void update() override {
      if (!prev_window_size.has_value()) { prev_window_size = ui.get_window_size(); }
      if (prev_window_size != ui.get_window_size()) {
        prev_window_size = ui.get_window_size();
        if (on_pressed) { on_pressed(); }
      }
      set_size(ui.get_window_size());
      set_is_drawn_on_top(true);
      Sprite::update();
    }

    void event(Input::InputEventMouseButton& ev) override {
      if (ev.action == Input::MouseAction::PRESS) {
        ev.handled = true;
        if (on_pressed) { on_pressed(); }
      }
    }

    void event(Input::InputEventMouseScroll& ev) override {
      ev.handled = true;
      if (on_pressed) { on_pressed(); }
    }

    void event(Input::InputEventMouseMove& ev) override { ev.handled = true; }

    void event(Input::InputEventKey& ev) override {
      if (ev.action == Input::KeyAction::RELEASE && ev.key == Input::Key::KEY_ENTER) {
        if (on_enter_pressed) { on_enter_pressed(); }
        ev.handled = true;
      } else if (ev.action == Input::KeyAction::RELEASE && ev.key == Input::Key::KEY_ESCAPE) {
        if (on_escape_pressed) { on_escape_pressed(); }
        ev.handled = true;
      }
    }

  public:
    std::optional<vec2i> prev_window_size{};
    std::function<void()> on_pressed{};
    std::function<void()> on_enter_pressed{};
    std::function<void()> on_escape_pressed{};
};

class PopupController : public Widget {
  public:
    PopupController(UI& ui_);
    void input() override;
    void update() override;

    void on_dimmer_pressed();
    void on_dimmer_enter_pressed();
    void on_dimmer_escape_pressed();

    void create_popover(const popover_descriptor& d);

    template <typename T, typename... Args>
      requires std::is_base_of_v<Popup, T>
    T* show_popup(Args&&... args) {
      close_all_popovers();
      auto& popup = add_child<T>(*this, [this](Popup* p) { on_popup_closed(p); }, std::forward<Args>(args)...);
      popups.emplace_back(&popup);
      return &popup;
    };

    bool is_popup_open() const;
    void close_all_popups();
    void close_all_popovers();

  protected:
    void on_popup_closed(Popup* popup);
    UI& ui;
    Dimmer* dimmer{};
    std::vector<Widget*> popups{};
    std::unordered_map<std::string, Widget*> popovers{};
};
