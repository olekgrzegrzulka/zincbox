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
#include "ui/zincgui/widget.hpp"

class Popup;
struct popover_descriptor;

namespace zincgui {
  class Root;

  namespace Input {
    struct InputEventMouseButton;
    struct InputEventMouseScroll;
    struct InputEventMouseMove;
    struct InputEventKey;
  } // namespace Input
} // namespace zincgui

class Dimmer : public zincgui::ColorRect {
  public:
    Dimmer(zincgui::Root& ui_);

    void update() override;
    void event(zincgui::Input::InputEventMouseButton& ev) override;
    void event(zincgui::Input::InputEventMouseScroll& ev) override;
    void event(zincgui::Input::InputEventMouseMove& ev) override;
    void event(zincgui::Input::InputEventKey& ev) override;

    void set_is_active(bool active);

  public:
    std::optional<vec2i> prev_window_size;
    std::function<void()> on_pressed;
    std::function<void()> on_enter_pressed;
    std::function<void()> on_escape_pressed;
};

class PopupController final : public zincgui::Widget {
  public:
    PopupController(zincgui::Root& ui_);
    void input() override;
    void update() override;
    void event(zincgui::Input::InputEventKey& e) override;

    void on_dimmer_pressed();
    void on_dimmer_enter_pressed();
    void on_dimmer_escape_pressed();

    void create_popover(const popover_descriptor& d);

    template <typename T, typename... Args>
      requires std::is_base_of_v<Popup, T>
    T* show_popup(Args&&... args) {
      close_all_popovers();
      auto& popup = add_child<T>(*this, [this](Popup* p) { on_popup_closed(p); }, std::forward<Args>(args)...);
      m_popups.emplace_back(&popup);
      return &popup;
    };

    bool is_popup_open() const;
    void close_all_popups();
    void close_all_popovers();

  private:
    void on_popup_closed(Popup* popup);
    Dimmer* m_dimmer{};
    std::vector<Widget*> m_popups;
    std::unordered_map<std::string, Widget*> m_popovers;
};
