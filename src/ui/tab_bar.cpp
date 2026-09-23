#include "tab_bar.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdlib.h>
#include "common/color.hpp"
#include "ui/zincgui/input.hpp"
#include "zincbox.hpp"
#include "theme_config.hpp"
#include "ui/theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"

using namespace zincgui;

Tab::Tab(Root& ui_, const tab_info& info) : Button(ui_) {
  label.set_text(info.label);
  label.update();
  set_width(std::clamp((i32)get_label().get_width() + 2 * info.padding, 40, 200));
  padding = info.padding;
  is_draggable = info.is_draggable;
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
  label.set_text_color(theme::config().text_color);
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
  label.set_text_color(theme::config().text_color_muted);
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

  const auto& text_color = theme::config().text_color;
  const auto& text_color_muted = theme::config().text_color_muted;
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

TabBar::TabBar(Root& ui_) : Widget(ui_) {
  set_layout("ltr expand fill");
  // set_clip_children(true);

  tab_container = &add_child<Widget>();
  tab_container->set_clip_children(true);

  auto& pad = add_child<Widget>();
  pad.set_min_width(6);
  pad.set_max_width(6);

  button_add = &add_child<ZincboxButton>("add_tab", theme::config().top_bar.button_add_tab);
  button_add->add_image("add_tab_icon");
  button_add->set_parent_anchor(Anchor::BOTTOM_LEFT);
  button_add->set_anchor(Anchor::BOTTOM_LEFT);
  button_add->on_press([this]() {
    if (on_add_tab_button_pressed) { on_add_tab_button_pressed(); };
  });

  static const float scale = zincbox::ui_scale();

  button_right = &add_child<Button>("");
  button_right->add_image("right");
  button_right->set_ignore_parents_layout(true);
  button_right->set_x(-2 * scale);
  button_right->set_parent_anchor(Anchor::CENTER_LEFT);
  button_right->set_anchor(Anchor::CENTER_RIGHT);

  button_left = &add_child<Button>("");
  button_left->add_image("left");
  button_left->set_ignore_parents_layout(true);
  button_left->set_x(2 * scale);
  button_left->set_parent_anchor(Anchor::CENTER_LEFT);
  button_left->set_anchor(Anchor::CENTER_LEFT);
}

void TabBar::add_tab(const tab_info& info, bool select) { add_tab(info, tabs.size(), select); }

void TabBar::add_tab(const tab_info& info, size_t at, bool select) {
  at = std::min(at, tabs.size());
  Tab* t = &tab_container->add_child<Tab>(info);
  t->set_height(height);
  t->on_drag_start = [this](i32 id) -> void { on_tab_drag_start(id); };
  t->on_active = [this, info](Tab* tab) -> void {
    i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - Input::get_mouse_x());
    if ((i32)tab->index == dragged_tab_index && mouse_drag_delta > 10) { return; }

    update_tab_textures(tab->index);

    for (Tab* t_ : tabs) {
      t_->active = false;
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

i32 TabBar::get_tab_container_width() { return tab_container->get_max_width(); }

double TabBar::get_max_scroll_px() { return std::max(0, get_tab_container_width() - width); }

void TabBar::scroll(double value) {
  value = std::clamp<double>(target_scroll_px + value, 0, get_max_scroll_px());
  if ((i32)target_scroll_px != (i32)value) { target_scroll_px = value; }
}

void TabBar::update() {
  button_add->set_min_width(button_add->get_width());
  button_add->set_max_width(button_add->get_width());
  button_add->set_min_height(button_add->get_height());
  button_add->set_max_height(button_add->get_height());

  target_scroll_px = std::clamp<double>(target_scroll_px, 0, get_max_scroll_px());
  auto scroll_px_ = scroll_px;
  double delta = std::abs(scroll_px - target_scroll_px);
  if (delta > 2.0f) {
    double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
    scroll_px = std::lerp(scroll_px, target_scroll_px, t);
  } else {
    scroll_px = scroll_px_;
  }
  if ((i32)scroll_px_ != (i32)scroll_px) { ui.mark_dirty_recursive(this); }

  if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) { dragged_tab_index = -1; }

  i32 mouse_x = Input::get_mouse_x() - x + scroll_px;
  i32 mouse_drag_delta = std::abs(drag_start_mouse_pos - Input::get_mouse_x());

  if (dragged_tab_index != -1 && mouse_drag_delta > 10) {
    if (dragged_tab_index < (i32)tabs.size() - 1) {
      i32 target_idx = -1;
      for (i32 i = dragged_tab_index + 1; i < (i32)tabs.size(); i += 1) {
        if (tabs[i]->is_draggable) {
          target_idx = i;
          break;
        }
      }

      if (target_idx != -1) {
        i32 sum = 0;
        for (i32 i = 0; i <= target_idx; i += 1) {
          sum += tabs[i]->get_width();
        }
        if (mouse_x > sum - tabs[target_idx]->get_width()) {
          if (swap_tabs(dragged_tab_index, target_idx)) { dragged_tab_index = target_idx; }
        }
      }
    }

    if (dragged_tab_index > 0) {
      i32 target_idx = -1;
      for (i32 i = dragged_tab_index - 1; i >= 0; i -= 1) {
        if (tabs[i]->is_draggable) {
          target_idx = i;
          break;
        }
      }

      if (target_idx != -1) {
        i32 sum = 0;
        for (i32 i = 0; i < target_idx; i += 1) {
          sum += tabs[i]->get_width();
        }
        if (mouse_x < sum + tabs[target_idx]->get_width()) {
          if (swap_tabs(dragged_tab_index, target_idx)) { dragged_tab_index = target_idx; }
        }
      }
    }
  }

  position_tabs(true);

  button_right->set_x(tab_container->get_width());
  button_left->set_is_drawn(target_scroll_px > 0);
  button_left->set_is_updated(button_left->get_is_drawn());
  button_right->set_is_drawn(target_scroll_px < get_max_scroll_px());
  button_right->set_is_updated(button_right->get_is_drawn());

  if (button_right->is_mouse_hovering() && button_right->get_is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    scroll(3);
  }

  if (button_left->is_mouse_hovering() && button_left->get_is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    scroll(-3);
  }

  scroll_px = std::clamp<double>(scroll_px, 0, get_max_scroll_px());

  static const float scale = zincbox::ui_scale();
  button_right->set_size(height - 4 * scale, height - 4 * scale);
  button_left->set_size(height - 4 * scale, height - 4 * scale);

  Widget::update();
}

void TabBar::position_tabs(bool smooth) {
  i32 tab_x = 0;
  for (i32 id = 0; id < (i32)tabs.size(); id += 1) {
    Tab* tab = tabs[id];
    if (id != dragged_tab_index) {
      tab->set_is_drawn_on_top(false);
      if (smooth) {
        tab->move_smooth(tab_x - scroll_px);
      } else {
        tab->move(tab_x - scroll_px);
      }
    } else {
      tab->set_is_drawn_on_top(true);
      i32 dragged_tab_x = drag_start_tab_pos + Input::get_mouse_x() - drag_start_mouse_pos;
      tab->move(dragged_tab_x);
    }

    tab_x += tab->get_width() - 1;
  }
  tab_container->set_max_width(tab_x + 28); // FIXME
}

void TabBar::update_tab_textures(i32 id) {
  for (Tab* t : tabs) {
    t->set_texture_inactive();
    t->set_draw_behind_parent(true);
  }
  selected_tab_index = id;
  if (tab_valid(selected_tab_index)) {
    tabs[selected_tab_index]->set_texture_active();
    tabs[selected_tab_index]->set_draw_behind_parent(false);
  }
}

void TabBar::skip_anim() { position_tabs(false); }

void TabBar::sort_tabs_by_label(std::span<const std::string> labels) {
  std::unordered_map<std::string, i32> label_priority;
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

  skip_anim();
}

const Tab* TabBar::get_tab_by_label(const std::string& label) const {
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

void TabBar::event(Input::InputEventMouseScroll& ev) {
  if (is_mouse_hovering()) {
    scroll(ev.offset.y * 15.0f);
    ev.handled = true;
  }
  if (!ev.handled) { Widget::event(ev); }
}
