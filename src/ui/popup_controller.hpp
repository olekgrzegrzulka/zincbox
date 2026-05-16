#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "common/debug.hpp"
#include "common/input.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class Popup;

struct popup_descriptor {
    std::string id;
    std::u32string_view title;
    std::optional<std::u32string_view> content;
    std::vector<std::u32string> button_labels;
    std::vector<std::function<void(Popup*)>> button_actions;
};

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
    Popup(UI& ui_) : Sprite(ui_, "panel_popup"),
                     title(add_child<Label>()), content(add_child<Widget>()) {
      set_is_drawn_on_top(true);
      set_nine_slice_margin(8.0f);
      set_parent_anchor(Anchor::CENTER);
      set_anchor(Anchor::CENTER);
      set_layout("ttb expand fit m:12 s:12");

      content.set_parent_anchor(Anchor::TOP);
      content.set_anchor(Anchor::TOP);
      content.set_layout("ttb expand fit m:0 s:4");
    }

  public:
    Label& title;
    Widget& content;
    std::function<void(Popup*)> on_confirm;
    std::function<void(Popup*)> on_cancel;
};

class Popover : public Sprite {
  public:
    Popover(UI& ui_) : Sprite(ui_, "popover_panel") {
    }

    void update() override {
      Sprite::update();
    }

    void draw() override {
      i32 off_screen_left = std::max(0, 0 - get_position(Anchor::TOP_LEFT).x);
      i32 off_screen_right = std::max(0, (get_position(Anchor::TOP_LEFT).x + width) - ui.get_window_width());
      i32 off_screen_top = std::max(0, 0 - get_position(Anchor::TOP_LEFT).y);
      i32 off_screen_bottom = std::max(0, (get_position(Anchor::TOP_LEFT).y + height) - ui.get_window_height());

      set_x(get_x() + off_screen_left - off_screen_right);
      set_y(get_y() + off_screen_top - off_screen_bottom);

      Sprite::draw();
    }

  protected:
    vec2i popover_pos;
};

class Dimmer : public Sprite {
  public:
    Dimmer(UI& ui_) : Sprite(ui_, "dim") {
    }

    void set_is_active(bool active) {
      set_is_updated(active);
      if (!active) {
        prev_window_size = std::nullopt;
      }
    }

    void update() override {
      if (!prev_window_size.has_value()) {
        prev_window_size = ui.get_window_size();
      }
      if (prev_window_size != ui.get_window_size()) {
        prev_window_size = ui.get_window_size();
        if (on_pressed) { on_pressed(); }
      }
      set_size(ui.get_window_size());
      set_is_drawn_on_top(true);
      Sprite::update();
    }

    void handle_event(Input::InputEventMouseButton& ev) override {
      if (ev.action == Input::MouseAction::PRESS) {
        ev.handled = true;
        if (on_pressed) { on_pressed(); }
      }
    }

    void handle_event(Input::InputEventMouseScroll& ev) override {
      ev.handled = true;
      if (on_pressed) { on_pressed(); }
    }

    void handle_event(Input::InputEventMouseMove& ev) override {
      ev.handled = true;
    }

    void handle_event(Input::InputEventKey& ev) override {
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
    PopupController(UI& ui_) : Widget(ui_), ui(ui_), dimmer(add_child<Dimmer>()) {
      dimmer.on_pressed = [this]() { on_dimmer_pressed(); };
      dimmer.on_enter_pressed = [this]() { on_dimmer_enter_pressed(); };
      dimmer.on_escape_pressed = [this]() { on_dimmer_escape_pressed(); };
    }

    void on_dimmer_pressed() {
      for (auto [_, p] : popovers) {
        p->set_marked_for_deletion(true);
      }
      popovers.clear();
    }

    void on_dimmer_enter_pressed() {
      for (auto [_, p] : popups) {
        if (p->on_confirm) { p->on_confirm(p); }
        p->set_marked_for_deletion(true);
      }
      popups.clear();
    }

    void on_dimmer_escape_pressed() {
      for (auto [_, p] : popovers) {
        p->set_marked_for_deletion(true);
      }
      popovers.clear();

      for (auto [_, p] : popups) {
        if (p->on_cancel) { p->on_cancel(p); }
        p->set_marked_for_deletion(true);
      }
      popups.clear();
    }

    void create_popover(popover_descriptor d) {
      ensure(d.button_labels.size() >= d.button_actions.size());
      if (popovers.contains(d.id)) { return; }

      i32 space_needed = 8 + (4 + 24) * d.button_labels.size() + 12;
      bool bottom = ui.get_window_height() - d.at.y >= space_needed;

      auto& popover = add_child<Popover>();
      popovers[d.id] = &popover;

      popover.set_is_drawn_on_top(true);
      popover.set_nine_slice_margin(8.0f);
      popover.set_anchor(bottom ? Anchor::TOP : Anchor::BOTTOM);
      popover.set_layout("ttb fit expand fill m:4 s:0");
      if (bottom) {
        popover.get_layout().direction = LayoutDirection::TOP_TO_BOTTOM;
      } else {
        popover.get_layout().direction = LayoutDirection::BOTTOM_TO_TOP;
      }

      popover.set_width(50);

      auto* arrow = &popover.add_child<Sprite>(bottom ? "popover_arrow" : "popover_arrow_inverted");
      arrow->set_ignore_parents_layout(true);
      arrow->set_parent_anchor(bottom ? Anchor::TOP : Anchor::BOTTOM);
      arrow->set_anchor(bottom ? Anchor::BOTTOM : Anchor::TOP);
      arrow->set_y(bottom ? 1 : -1);
      arrow->set_is_drawn(d.show_arrow);
      if (bottom) {
        popover.set_pos(d.at.x, d.at.y + (d.distance + arrow->get_height()));
      } else {
        popover.set_pos(d.at.x, d.at.y - (d.distance + arrow->get_height()));
      }
      std::vector<Button*> buttons;
      for (auto& sv : d.button_labels) {
        auto& btn = popover.add_child<Button>(std::string(sv));
        buttons.emplace_back(&btn);
        btn.set_nine_slice_margin(8.0f);
        btn.set_texture_idle("button_popover_idle");
        btn.set_texture_hovered("button_popover_hovered");
        btn.set_texture_pressed("button_popover_pressed");
        btn.set_texture_disabled("button_popover_disabled");
        btn.set_texture("button_popover_idle", false);
        btn.set_min_height(22);
        btn.set_max_height(22);
        btn.set_height(22);
        popover.set_width(std::max(popover.get_width(), btn.get_label().get_width() + 30));
      }

      size_t button_i = 0;
      std::string popover_id = d.id;
      for (auto& l : d.button_actions) {
        auto lambda = [this, l, popover_id, &popover]() {
          if (l) { l(); }
          popovers.erase(popover_id);
          popover.set_marked_for_deletion(true);
        };
        buttons[button_i]->on_press(lambda);
        button_i += 1;
      }
    };

    Popup* create_popup(popup_descriptor d) {
      ensure(d.button_actions.size() >= d.button_actions.size());
      if (popups.contains(d.id)) { return nullptr; }

      auto& popup = add_child<Popup>();
      auto popup_id = d.id;
      popups[popup_id] = &popup;

      popup.title.set_text(d.title);
      if (d.content.has_value()) {
        auto& content_label = popup.content.add_child<Label>(std::u32string{*d.content});
        popup.set_width(std::max(popup.title.get_width(), content_label.get_width()) + 20);
      } else {
        popup.set_width(popup.title.get_width() + 20);
      }

      auto& button_container = popup.add_child<Widget>();
      button_container.set_height(30);
      button_container.set_layout("ltr fill expand m:0 s:4");
      std::vector<Button*> buttons;
      for (auto& sv : d.button_labels) {
        auto& btn = button_container.add_child<Button>(std::u32string(sv));
        buttons.emplace_back(&btn);
      }

      size_t button_i = 0;
      for (auto& l : d.button_actions) {
        auto lambda = [this, l, &popup, popup_id]() {
          if (l) { l(&popup); }
          popups.erase(popup_id);
          popup.set_marked_for_deletion(true);
        };
        buttons[button_i]->on_press(lambda);
        button_i += 1;
      }

      return &popup;
    };

    bool is_popup_open() const {
      return popups.size() > 0;
    }

    void close_all_popups() {
      for (auto [popup_id, popup] : popups) {
        popup->set_marked_for_deletion(true);
      }
      popups.clear();
    }

    void process_input() override {
      // copy due to iterator invalidation if popup closes
      // FIXME get rid of this, maybe use weak_ptr<Widget> instead of Widget*
      auto popups_copy = popups;
      for (auto [_, w] : popups_copy) {
        w->process_input();
      }
      auto popovers_copy = popovers;
      for (auto [_, w] : popovers_copy) {
        w->process_input();
      }
      if (dimmer.get_is_updated()) { dimmer.process_input(); }
    }

    void update() override {
      set_size(ui.get_window_size());
      bool dimmer_block_events = (popovers.size() + popups.size()) > 0;
      bool dimmer_visible = popups.size() > 0;
      dimmer.set_is_active(dimmer_block_events);
      dimmer.set_is_drawn(dimmer_visible);
      Widget::update();
    }

  protected:
    UI& ui;
    Dimmer& dimmer;
    std::unordered_map<std::string, Popup*> popups;
    std::unordered_map<std::string, Widget*> popovers;
};
