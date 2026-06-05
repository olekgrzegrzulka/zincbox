#include "tab_bar.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include "common/input.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

Tab::Tab(UI& ui_) : Button(ui_), label(add_child<Label>()) {
  label.set_anchor(Anchor::CENTER);
  label.set_parent_anchor(Anchor::CENTER);
  label.set_label_anchor(Anchor::CENTER);

  set_clip_children(true);
  set_texture_inactive();
  set_nine_slice_margin(8.0f);

  on_press([&]() {
    on_active(this);
  });

  on_press_rmb([this]() {
    if (on_right_click) { on_right_click(this); }
  });
}

void Tab::set_texture_active() {
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

void Tab::set_texture_inactive() {
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

void Tab::move_smooth(i32 to) {
  if (just_added) {
    move(to);
    just_added = false;
    return;
  }

  if (to != x_new) {
    x_old = x;
    x_new = to;
    t = 0.0f;
  }
}

void Tab::move(i32 to) {
  x_old = to;
  x_new = to;
  t = 1.0f;
}

void Tab::update() {
  set_width(std::clamp((i32)label.get_width() + 2 * padding, 40, 200));
  t = std::clamp(t + 0.2f, 0.0f, 1.0f);
  set_x(std::lerp(x_old, x_new, std::sin(1.5708 * t)));
  Button::update();
}

void Tab::event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;
  bool handled = false;
  if (is_mouse_hovering() && ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS) {
    handled = true;
    if (on_drag_start) {
      on_drag_start(index);
    }
  }

  Button::event(ev);
  ev.handled = ev.handled || handled;
}

TabBar::TabBar(UI& ui_) : Sprite(ui_, "panel_tabbar") {
  set_height(theme::get_prop("top_bar_height").as_i32(32));
  set_layout("fit");
  tab_container = &add_child<Widget>();
  tab_container->set_height(height);
  button_add = &add_child<ZincboxButton>("add_tab");
  button_add->set_ignore_parents_layout(true);
  button_add->set_nine_slice_margin(0.0f);
  button_add->set_parent_anchor(Anchor::BOTTOM_LEFT);
  button_add->set_anchor(Anchor::BOTTOM_LEFT);
  button_add->on_press([this]() {
    if (on_add_tab_button_pressed) { on_add_tab_button_pressed(); };
  });
}

void TabBar::add_tab(tab_info info, bool select) {
  add_tab(info, tabs.size(), select);
}

void TabBar::add_tab(tab_info info, size_t at, bool select) {
  at = std::min(at, tabs.size());
  Tab* t = &tab_container->add_child<Tab>();
  t->set_height(height);
  t->label.set_text(info.label);
  t->padding = info.padding;
  t->is_draggable = info.is_draggable;
  t->on_drag_start = [this](i32 id) { on_tab_drag_start(id); };
  t->on_active = [this, info](Tab* tab) {
    i32 mouse_x = Input::get_mouse_x() - x;
    i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - mouse_x - x);
    if ((i32)tab->index == dragged_tab_index && mouse_drag_delta > 10) { return; }

    update_tab_textures(tab->index);
    if (info.on_open) { info.on_open(); }
    if (on_tab_pressed) { on_tab_pressed(info.id); }
  };
  t->on_right_click = info.on_right_click;

  tabs.emplace(tabs.begin() + at, t);
  t->index = at;
  t->id = info.id;

  for (size_t i = t->index; i < tabs.size(); i += 1) {
    tabs[i]->index = i;
  }

  if (select) {
    update_tab_textures(t->index);
    if (info.on_open) { info.on_open(); }
    if (on_tab_pressed) { on_tab_pressed(info.id); }
  }
}

void TabBar::open_tab(i32 id) {
  if (tab_valid(selected_tab_index) && tabs[selected_tab_index]->id == id) {
    return;
  }
  auto it = std::find_if(tabs.begin(), tabs.end(), [id](Tab* t) { return t->id == id; });
  if (it != tabs.end()) {
    Tab* t = *it;
    t->on_active(t);
  }
}

void TabBar::close_tab(i32 id) {
  auto it = std::find_if(tabs.begin(), tabs.end(), [id](Tab* t) { return t->id == id; });
  if (it != tabs.end()) {
    tabs.erase(it);
  }
}

void TabBar::close_all_tabs() {
  for (auto& t : tab_container->get_children()) {
    t->set_marked_for_deletion(true);
  }
  tabs.clear();
}

void TabBar::update() {
  if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    dragged_tab_index = -1;
  }

  i32 mouse_x = Input::get_mouse_x() - x;
  i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - mouse_x - x);

  if (dragged_tab_index != -1 && mouse_drag_delta > 10) {
    if (dragged_tab_index + 1 < (i32)tabs.size()) {
      Tab* next = tabs[dragged_tab_index + 1];
      i32 sum = 0;
      for (i32 i = 0; i < dragged_tab_index; i += 1) {
        sum += tabs[i]->get_width();
      }
      sum += next->get_width();
      if (mouse_x > sum && mouse_x < sum + tabs[dragged_tab_index]->get_width()) {
        if (selected_tab_index == dragged_tab_index) {
          selected_tab_index += 1;
        }
        if (swap_tabs(dragged_tab_index, dragged_tab_index + 1)) {
          dragged_tab_index += 1;
        }
      }
    }
    if (dragged_tab_index - 1 >= 0) {
      Tab* prev = tabs[dragged_tab_index - 1];
      i32 sum = 0;
      for (i32 i = 0; i < dragged_tab_index - 1; i += 1) {
        sum += tabs[i]->get_width();
      }
      if (mouse_x > sum && mouse_x < sum + prev->get_width()) {
        if (selected_tab_index == dragged_tab_index) {
          selected_tab_index -= 1;
        }
        if (swap_tabs(dragged_tab_index, dragged_tab_index - 1)) {
          dragged_tab_index -= 1;
        }
      }
    }
  }

  i32 tab_x = 0;
  for (i32 id = 0; id < (i32)tabs.size(); id += 1) {
    Tab* tab = tabs[id];
    if (id != dragged_tab_index) {
      tab->set_is_drawn_on_top(false);
      // if (tab->just_added) {
      //   tab->move(tab_x);
      //   tab->just_added = false;
      // } else {
      tab->move_smooth(tab_x);
      // }

    } else {
      tab->set_is_drawn_on_top(true);
      i32 dragged_tab_x = drag_start_tab_pos + Input::get_mouse_x() - drag_start_mouse_pos;
      tab->move(dragged_tab_x);
    }

    tab_x += tab->get_width() - 1;
  }

  tab_container->set_width(tab_x);
  button_add->set_x(tab_x + 4);
  Sprite::update();
}

void TabBar::update_tab_textures(i32 id) {
  for (Tab* t : tabs) {
    t->set_texture_inactive();
  }
  selected_tab_index = id;
  if (tab_valid(selected_tab_index)) {
    tabs[selected_tab_index]->set_texture_active();
  }
}

void TabBar::on_tab_drag_start(i32 id) {
  if (tab_valid(id) && tabs[id]->is_draggable) {
    dragged_tab_index = id;
    drag_start_mouse_pos = Input::get_mouse_x();
    drag_start_tab_pos = tabs[id]->get_x();
  }
}

bool TabBar::tab_valid(size_t index) const {
  return index < tabs.size();
}

bool TabBar::swap_tabs(size_t index_a, size_t index_b) {
  if (!tab_valid(index_a) || !tab_valid(index_a)) { return false; }
  if (!tabs[index_a]->is_draggable || !tabs[index_b]->is_draggable) { return false; }

  std::swap(tabs[index_a], tabs[index_b]);
  std::swap(tabs[index_a]->index, tabs[index_b]->index);
  return true;
}
