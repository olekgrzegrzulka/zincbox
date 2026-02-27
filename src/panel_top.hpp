#pragma once
#include <functional>
#include <vector>
#include "input.hpp"
#include "interface.hpp"
#include "musicdb.hpp"
#include "popup_controller.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

class Tab : public Button {
public:
  Tab(UI& ui_) : Button(ui_), label(add_child<Label>()) {
    label.set_anchor(Anchor::CENTER);
    label.set_parent_anchor(Anchor::CENTER);

    set_texture_inactive();
    set_nine_slice_margin(8.0f);

    on_press([&]() {
      on_active(id);
    });
  }

  void set_texture_active() {
    set_texture_disabled("tab_active_disabled");
    set_texture_hovered("tab_active_hovered");
    set_texture_idle("tab_active_idle");
    set_texture_pressed("tab_active_pressed");
    if (state == ButtonState::DISABLED) {
      set_sprite_disabled();
    } else if (mouse_hovering) {
      set_sprite_hovered();
    } else {
      set_sprite_idle();
    }
  }

  void set_texture_inactive() {
    set_texture_disabled("tab_inactive_disabled");
    set_texture_hovered("tab_inactive_hovered");
    set_texture_idle("tab_inactive_idle");
    set_texture_pressed("tab_inactive_pressed");
    if (state == ButtonState::DISABLED) {
      set_sprite_disabled();
    } else if (mouse_hovering) {
      set_sprite_hovered();
    } else {
      set_sprite_idle();
    }
  }

  void move_smooth(i32 to) {
    if (to != x_new) {
      x_old = x;
      x_new = to;
      t = 0.0f;
    }
  }

  void move(i32 to) {
    x_old = to;
    x_new = to;
    t = 1.0f;
  }

  void update() override {
    set_width(std::clamp((i32)label.get_text_extents().x + 20, 100, 200));
    t = std::clamp(t + 0.2f, 0.0f, 1.0f);
    set_x(std::lerp(x_old, x_new, std::sin(1.5708 * t)));
    Button::update();
  }

  void handle_event(Input::InputEventMouseButton& ev) override {
    using enum Input::MouseButton;
    using enum Input::MouseAction;
    bool handled = false;
    if (is_mouse_hovering() && ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS) {
      handled = true;
      if (on_drag_start) {
        on_drag_start(id);
      }
    }

    Button::handle_event(ev);
    ev.handled = ev.handled || handled;
  }

public:
  Label& label;
  size_t id = 0;
  std::function<void(i32)> on_drag_start{};
  std::function<void(i32)> on_active{};

  i32 x_old = 0;
  i32 x_new = 0;
  float t = 0.0f;
  bool just_added = true;
};

class PanelTop : public Panel {
public:
  PanelTop(UI& ui_) : Panel(ui_) {
    set_height(30);
    tab_container = &add_child<Widget>();
    tab_container->set_height(30);
    button_add = &add_child<Button>("+");
    button_add->set_max_width(24);
    button_add->set_size(24, 24);
    button_add->set_max_height(24);
    button_add->set_parent_anchor(Anchor::BOTTOM_LEFT);
    button_add->set_anchor(Anchor::BOTTOM_LEFT);
    button_add->on_press([this]() {
      popover_descriptor d{
        .id = "x",
        .at = button_add->get_position(Anchor::BOTTOM),
        .button_labels = {"New album collection...", "New playlist collection..."},
        .button_actions = {[]() {}, []() {}},
      };
      interface::get_popup_controller()->create_popover(d);
    });
  }

  void recreate() {
    for (auto& t : tab_container->get_children()) {
      t->set_marked_for_deletion(true);
    }
    tabs.clear();

    for (musicdb::collection_id_t i = 0; i < musicdb::get_collections().size(); i += 1) {
      add_tab(i);
    }
  }

  void add_tab(musicdb::collection_id_t collection_id) {
    musicdb::Collection* collection = musicdb::get_collection(collection_id);

    Tab* t = &tab_container->add_child<Tab>();
    t->set_height(30);
    t->label.set_text(collection->get_name());
    t->id = collection_id;
    t->on_drag_start = [this](i32 id) { on_tab_drag_start(id); };
    t->on_active = [this, collection_id](i32 id) { on_tab_pressed(id); on_collection_opened(collection_id); };
    tabs.emplace_back(t);
  }

  void update() override {
    if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
      dragged_tab = -1;
    }

    i32 mouse_x = Input::get_mouse_x() - x;
    i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - mouse_x - x);

    if (dragged_tab != -1 && mouse_drag_delta > 10) {
      if (dragged_tab + 1 < (i32)tabs.size()) {
        Tab* next = tabs[dragged_tab + 1];
        i32 sum = 0;
        for (int i = 0; i < dragged_tab; i += 1) {
          sum += tabs[i]->get_width();
        }
        sum += next->get_width();
        if (mouse_x > sum && mouse_x < sum + tabs[dragged_tab]->get_width()) {
          if (selected_tab == dragged_tab) {
            selected_tab += 1;
          }
          std::swap(tabs[dragged_tab], tabs[dragged_tab + 1]);
          std::swap(tabs[dragged_tab]->id, tabs[dragged_tab + 1]->id);
          dragged_tab += 1;
        }
      }
      if (dragged_tab - 1 >= 0) {
        Tab* prev = tabs[dragged_tab - 1];
        i32 sum = 0;
        for (int i = 0; i < dragged_tab - 1; i += 1) {
          sum += tabs[i]->get_width();
        }
        if (mouse_x > sum && mouse_x < sum + prev->get_width()) {
          if (selected_tab == dragged_tab) {
            selected_tab -= 1;
          }
          std::swap(tabs[dragged_tab], tabs[dragged_tab - 1]);
          std::swap(tabs[dragged_tab]->id, tabs[dragged_tab - 1]->id);
          dragged_tab -= 1;
        }
      }
    }

    i32 tab_x = 0;
    for (i32 id = 0; id < (i32)tabs.size(); id += 1) {
      Tab* tab = tabs[id];
      if (id != dragged_tab) {
        tab->set_is_drawn_on_top(false);
        if (tab->just_added) {
          tab->move(tab_x);
          tab->just_added = false;
        } else {
          tab->move_smooth(tab_x);
        }

      } else {
        tab->set_is_drawn_on_top(true);
        i32 dragged_tab_x = drag_start_tab_pos + Input::get_mouse_x() - drag_start_mouse_pos;
        tab->move(dragged_tab_x);
      }

      tab_x += tab->get_width() - 1;
    }

    tab_container->set_width(tab_x);
    button_add->set_x(tab_x + 4);
    Panel::update();
  }

  void on_tab_pressed(i32 id) {
    i32 mouse_x = Input::get_mouse_x() - x;
    i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - mouse_x - x);
    if (id == dragged_tab && mouse_drag_delta > 10) { return; }

    for (Tab* t : tabs) {
      t->set_texture_inactive();
    }
    selected_tab = id;
    if (tab_valid(selected_tab)) {
      tabs[selected_tab]->set_texture_active();
    }
  }

  void on_tab_drag_start(i32 id) {
    if (tab_valid(id)) {
      dragged_tab = id;
      drag_start_mouse_pos = Input::get_mouse_x();
      drag_start_tab_pos = tabs[id]->get_x();
    }
  }

  bool tab_valid(i32 id) const {
    return id >= 0 && id < (i32)tabs.size();
  }

public:
  std::function<void(musicdb::collection_id_t)> on_collection_opened{};

protected:
  Widget* tab_container{};
  Button* button_add{};
  i32 selected_tab = -1;
  std::vector<Tab*> tabs;
  i32 dragged_tab = -1;
  i32 drag_start_mouse_pos = 0;
  i32 drag_start_tab_pos = 0;
};
