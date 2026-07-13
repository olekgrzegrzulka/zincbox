#include "tab_bar.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/input.hpp"
#include "ui/theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

Tab::Tab(UI& ui_) : Button(ui_) {
  label.set_anchor(Anchor::CENTER);
  label.set_parent_anchor(Anchor::CENTER);
  label.set_label_anchor(Anchor::CENTER);

  set_clip_children(true);
  set_texture_inactive();
  set_nine_slice_margin(8.0f);

  on_press([&]() { on_active(this); });

  on_press_rmb([this]() {
    if (on_right_click) { on_right_click(this); }
  });
}

void Tab::set_texture_active() {
  set_texture_disabled("tab_active_disabled");
  set_texture_hovered("tab_active_hovered");
  set_texture_idle("tab_active_idle");
  set_texture_pressed("tab_active_pressed");
  label.set_text_color(theme::get_prop("text_color").as_rgba());
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
  label.set_text_color(theme::get_prop("text_color_muted").as_rgba());
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

  static const rgba text_color = theme::get_prop("text_color").as_rgba();
  static const rgba text_color_muted = theme::get_prop("text_color_muted").as_rgba();
  get_label().set_text_color(active ? text_color : text_color_muted);
}

void Tab::event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;

  Button::event(ev);
  if (is_mouse_hovering() && ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS) {
    ev.handled = true;
    if (on_drag_start) { on_drag_start(index); }
  }
}

TabBar::TabBar(UI& ui_) : Sprite(ui_, "panel_tabbar") {
  set_height(theme::get_prop("top_bar_height").as_i32(32));
  set_layout("fit");
  tab_container = &add_child<Widget>();
  tab_container->set_height(height);
  button_add = &add_child<ZincboxButton>("add_tab");
  button_add->set_nine_slice_margin(0.0f);
  button_add->set_parent_anchor(Anchor::BOTTOM_LEFT);
  button_add->set_anchor(Anchor::BOTTOM_LEFT);
  button_add->on_press([this]() {
    if (on_add_tab_button_pressed) { on_add_tab_button_pressed(); };
  });
}

void TabBar::add_tab(const tab_info& info, bool select) { add_tab(info, tabs.size(), select); }

void TabBar::add_tab(const tab_info& info, size_t at, bool select) {
  at = std::min(at, tabs.size());
  Tab* t = &tab_container->add_child<Tab>();
  t->set_height(height);
  t->get_label().set_text(info.label);
  t->padding = info.padding;
  t->is_draggable = info.is_draggable;
  t->on_drag_start = [this](i32 id) { on_tab_drag_start(id); };
  t->on_active = [this, info](Tab* tab) {
    i32 mouse_x = Input::get_mouse_x() - x;
    i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - mouse_x - x);
    if ((i32)tab->index == dragged_tab_index && mouse_drag_delta > 10) { return; }

    update_tab_textures(tab->index);

    for (Tab* t : tabs) {
      t->active = false;
    }

    tab->active = true;

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

  if ((i32)at <= selected_tab_index) { selected_tab_index += 1; }

  if (select) {
    update_tab_textures(t->index);
    if (info.on_open) { info.on_open(); }
    if (on_tab_pressed) { on_tab_pressed(info.id); }
  }
}

void TabBar::select_tab(i32 id) {
  if (tab_valid(selected_tab_index) && tabs[selected_tab_index]->id == id) { return; }
  auto it = std::find_if(tabs.begin(), tabs.end(), [id](Tab* t) { return t->id == id; });
  if (it != tabs.end()) {
    Tab* t = *it;
    t->on_active(t);
  }
}

void TabBar::unselect_all_tabs() {
  for (Tab* t : tabs) {
    t->active = false;
  }
  selected_tab_index = -1;
}

void TabBar::close_tab(i32 id) {
  auto it = std::find_if(tabs.begin(), tabs.end(), [id](Tab* t) { return t->id == id; });
  if (it != tabs.end()) {
    size_t removed_index = (*it)->index;
    (*it)->set_marked_for_deletion(true);
    tabs.erase(it);

    for (size_t i = 0; i < tabs.size(); i += 1) {
      tabs[i]->index = i;
    }

    if (selected_tab_index == (i32)removed_index) {
      selected_tab_index = -1;
    } else if (selected_tab_index > (i32)removed_index) {
      selected_tab_index -= 1;
    }
  }
}

void TabBar::close_all_tabs() {
  for (auto& t : tab_container->get_children()) {
    t->set_marked_for_deletion(true);
  }
  tabs.clear();
  selected_tab_index = -1;
}

void TabBar::update() {
  if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) { dragged_tab_index = -1; }

  i32 mouse_x = Input::get_mouse_x() - x;
  i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - Input::get_mouse_x());

  if (dragged_tab_index != -1 && mouse_drag_delta > 10) {
    if (dragged_tab_index < (i32)tabs.size() - 1) {
      Tab* next = tabs[dragged_tab_index + 1];
      i32 sum = 0;
      for (i32 i = 0; i < dragged_tab_index; i += 1) {
        sum += tabs[i]->get_width();
      }
      sum += next->get_width();
      if (mouse_x > sum) {
        if (swap_tabs(dragged_tab_index, dragged_tab_index + 1)) { dragged_tab_index += 1; }
      }
    }
    if (dragged_tab_index > 0) {
      Tab* prev = tabs[dragged_tab_index - 1];
      i32 sum = 0;
      for (i32 i = 0; i < dragged_tab_index - 1; i += 1) {
        sum += tabs[i]->get_width();
      }
      if (mouse_x < sum + prev->get_width()) {
        if (swap_tabs(dragged_tab_index, dragged_tab_index - 1)) { dragged_tab_index -= 1; }
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
  Sprite::update();
}

void TabBar::update_tab_textures(i32 id) {
  for (Tab* t : tabs) {
    t->set_texture_inactive();
  }
  selected_tab_index = id;
  if (tab_valid(selected_tab_index)) { tabs[selected_tab_index]->set_texture_active(); }
}

void TabBar::sort_tabs_by_label(std::span<const std::u32string> labels) {
  std::unordered_map<std::u32string, i32> label_priority;
  for (size_t i = 0; i < labels.size(); i += 1) {
    label_priority[labels[i]] = i;
  }

  std::sort(tabs.begin(), tabs.end(), [&](Tab* a, Tab* b) -> bool {
    auto it_lhs = label_priority.find(a->get_label().get_text());
    auto it_rhs = label_priority.find(b->get_label().get_text());
    if (it_lhs == label_priority.end()) { return false; }
    if (it_rhs == label_priority.end()) { return false; }
    return it_lhs->second < it_rhs->second;
  });

  for (size_t i = 0; i < tabs.size(); i += 1) {
    tabs[i]->index = i;
  }
  selected_tab_index = -1;
}

const Tab* TabBar::get_tab_by_label(const std::u32string& label) const {
  for (const Tab* tab : tabs) {
    if (tab->get_label().get_text() == label) { return tab; }
  }
  return nullptr;
}

void TabBar::on_tab_drag_start(i32 id) {
  if (tab_valid(id) && tabs[id]->is_draggable) {
    dragged_tab_index = id;
    drag_start_mouse_pos = Input::get_mouse_x();
    drag_start_tab_pos = tabs[id]->get_x();
  }
}

bool TabBar::tab_valid(size_t index) const { return index < tabs.size(); }

bool TabBar::swap_tabs(size_t index_a, size_t index_b) {
  if (!tab_valid(index_a) || !tab_valid(index_b)) { return false; }
  if (!tabs[index_a]->is_draggable || !tabs[index_b]->is_draggable) { return false; }

  std::swap(tabs[index_a], tabs[index_b]);
  std::swap(tabs[index_a]->index, tabs[index_b]->index);

  if (selected_tab_index == (i32)index_a) {
    selected_tab_index = index_b;
  } else if (selected_tab_index == (i32)index_b) {
    selected_tab_index = index_a;
  }
  return true;
}
