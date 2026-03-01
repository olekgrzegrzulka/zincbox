#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "debug.hpp"
#include "input.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

struct popup_descriptor {
    std::string id;
    std::string_view title;
    std::string_view content;
    std::initializer_list<std::string_view> button_labels;
    std::initializer_list<std::function<void()>> button_actions;
};

struct popover_descriptor {
    std::string id;
    vec2i at;
    std::initializer_list<std::string_view> button_labels;
    std::initializer_list<std::function<void()>> button_actions;
};

class Dimmer : public Sprite {
  public:
    Dimmer(UI& ui_) : Sprite(ui_, "dim") {
    }

    void update() override {
      set_width(ui.get_window_width());
      set_height(ui.get_window_height());
      set_is_drawn_on_top(true);
      Sprite::update();
    }

    void handle_event(Input::InputEventMouseButton& ev) override {
      if (ev.action == Input::MouseAction::PRESS) {
        ev.handled = true;
        if (on_pressed) { on_pressed(); }
      }
    }

  public:
    std::function<void()> on_pressed{};
};

class PopupConfirmAddPathsToCollection : public Panel {
  public:
    PopupConfirmAddPathsToCollection(UI& ui_, const std::vector<std::string>& paths) : Panel(ui_) {
      set_layout("ttb fit expand fill m:8 s:4");
      add_child<Label>("Do you want to add the following paths to collection '");
      for (auto& p : paths) {
        add_child<Label>(p);
      }
      auto& buttons = add_child<Widget>();
      set_layout("ltr fit expand fill m:0 s:4");
      buttons.add_child<Button>("Yes");
      buttons.add_child<Button>("No");
    }
};

class PopupController : public Widget {
  public:
    PopupController(UI& ui_) : Widget(ui_), ui(ui_), dimmer(add_child<Dimmer>()) {
      dimmer.on_pressed = [this]() { on_dimmer_pressed(); };
    }

    void on_dimmer_pressed() {
      for (auto [_, p] : popovers) {
        p->set_marked_for_deletion(true);
      }
      popovers.clear();
    }

    void create_popover(popover_descriptor d) {
      ensure(d.button_actions.size() >= d.button_actions.size());
      if (popovers.contains(d.id)) { return; }

      auto& popover = add_child<Sprite>("popover_panel");
      popovers[d.id] = &popover;

      popover.set_is_drawn_on_top(true);
      popover.set_nine_slice_margin(8.0f);
      popover.set_anchor(Anchor::TOP);
      popover.set_pos(d.at);
      popover.set_layout("ttb fit expand fill m:6 s:6");
      popover.set_width(50);

      auto* arrow = &popover.add_child<Sprite>("popover_arrow");
      arrow->set_ignore_parents_layout(true);
      arrow->set_parent_anchor(Anchor::TOP);
      arrow->set_anchor(Anchor::BOTTOM);
      arrow->set_y(4);
      popover.set_y(popover.get_y() + arrow->get_height());
      std::vector<Button*> buttons;
      for (auto& sv : d.button_labels) {
        auto& btn = popover.add_child<Button>(std::string(sv));
        buttons.emplace_back(&btn);
        btn.set_texture_idle("button_popover_idle");
        btn.set_texture_hovered("button_popover_hovered");
        btn.set_texture_pressed("button_popover_pressed");
        btn.set_texture_disabled("button_popover_disabled");
        btn.set_texture("button_popover_idle", false);
        btn.set_min_height(30);
        btn.set_max_height(30);
        btn.set_height(30);
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

    void create_popup(popup_descriptor d) {
      ensure(d.button_actions.size() >= d.button_actions.size());
      if (popups.contains(d.id)) { return; }

      auto& popup = add_child<Panel>(Panel::PanelStyle::Rectangular, true);
      auto popup_id = d.id;
      popups[popup_id] = &popup;

      popup.set_is_drawn_on_top(true);
      popup.set_nine_slice_margin(8.0f);
      popup.set_parent_anchor(Anchor::CENTER);
      popup.set_anchor(Anchor::CENTER);
      popup.set_layout("ttb expand fit m:15 s:15");

      auto& title = popup.add_child<Label>(std::string{d.title});
      auto& content = popup.add_child<Label>(std::string{d.content});

      popup.set_width(std::max(title.get_width(), content.get_width()) + 20);

      auto& button_container = popup.add_child<Widget>();
      button_container.set_height(30);
      button_container.set_layout("ltr fill expand m:0 s:8");
      std::vector<Button*> buttons;
      for (auto& sv : d.button_labels) {
        auto& btn = button_container.add_child<Button>(std::string(sv));
        buttons.emplace_back(&btn);
      }

      size_t button_i = 0;
      for (auto& l : d.button_actions) {
        auto lambda = [this, l, &popup, popup_id]() {
          if (l) { l(); }
          popups.erase(popup_id);
          popup.set_marked_for_deletion(true);
        };
        buttons[button_i]->on_press(lambda);
        button_i += 1;
      }
    };

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
      set_width(ui.get_window_width());
      set_height(ui.get_window_height());
      bool dimmer_block_events = (popovers.size() + popups.size()) > 0;
      bool dimmer_visible = popups.size() > 0;
      dimmer.set_is_updated(dimmer_block_events);
      dimmer.set_is_drawn(dimmer_visible);
      Widget::update();
    }

  protected:
    UI& ui;
    Dimmer& dimmer;
    std::unordered_map<std::string, Widget*> popups;
    std::unordered_map<std::string, Widget*> popovers;
};
