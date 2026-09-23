#pragma once
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "common/types.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"

class Popup;
struct popover_descriptor;

class Dimmer : public zincgui::ColorRect {
  public:
    Dimmer(zincgui::Root& ui_) : ColorRect(ui_, 0x00000000) {}

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
      ColorRect::update();
    }

    void event(zincgui::Input::InputEventMouseButton& ev) override {
      if (ev.action == zincgui::Input::MouseAction::PRESS) {
        ev.handled = true;
        if (on_pressed) { on_pressed(); }
      }
    }

    void event(zincgui::Input::InputEventMouseScroll& ev) override {
      ev.handled = true;
      if (on_pressed) { on_pressed(); }
    }

    void event(zincgui::Input::InputEventMouseMove& ev) override { ev.handled = true; }

    void event(zincgui::Input::InputEventKey& ev) override {
      if (ev.action == zincgui::Input::KeyAction::RELEASE && ev.key == zincgui::Input::Key::KEY_ENTER) {
        if (on_enter_pressed) { on_enter_pressed(); }
        ev.handled = true;
      } else if (ev.action == zincgui::Input::KeyAction::RELEASE && ev.key == zincgui::Input::Key::KEY_ESCAPE) {
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

class PopupController : public zincgui::Widget {
  public:
    PopupController(zincgui::Root& ui_);
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
    zincgui::Root& ui;
    Dimmer* dimmer{};
    std::vector<Widget*> popups{};
    std::unordered_map<std::string, Widget*> popovers{};
};
