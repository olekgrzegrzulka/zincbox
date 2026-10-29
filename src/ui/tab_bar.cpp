#include "tab_bar.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdlib.h>
#include "common/color.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

Tab::Tab(Root& ui_, const tab_info& info) : Button(ui_) {
  m_padding = info.padding;
  m_is_draggable = info.is_draggable;

  label().text(info.label);
  width(std::clamp(label().text_extents().x + (2 * m_padding), 40, 200));

  label().anchor(Anchor::CENTER).parent_anchor(Anchor::CENTER).label_anchor(Anchor::CENTER);
  active(false);

  clip_children(true);
  texture_inactive();
  nine_slice_margin((i32)(theme::config().panel_top.height / 2));

  on_press([this]() {
    if (m_on_active) { m_on_active(this); }
  });

  on_press_rmb([this]() {
    if (m_on_right_click) { m_on_right_click(this); }
  });
}

void Tab::update() {
  m_t = std::clamp(m_t + 0.2f, 0.0f, 1.0f);
  if (m_t > 0.99f) {
    x(m_x_new);
  } else {
    x(std::lerp(m_x_old, m_x_new, std::sin(1.5708f * m_t)));
  }

  Button::update();
}

void Tab::event(Input::InputEventMouseButton& ev) {
  using enum Input::MouseButton;
  using enum Input::MouseAction;

  Button::event(ev);
  if (is_mouse_hovering() && ev.button == MOUSE_BUTTON_LEFT && ev.action == PRESS) {
    ev.handled = true;
    if (m_on_drag_start) { m_on_drag_start(m_index); }
  }
}

void Tab::on_state_changed(ButtonState /* prev_state */) {
  const auto& text_color = theme::config().text_color;
  const auto& text_color_muted = theme::config().text_color_muted;

  if (state() == ButtonState::HOVERED) {
    sprite_hovered();
    label().text_color(m_active ? text_color : text_color_muted);
  } else if (state() == ButtonState::IDLE) {
    sprite_idle();
    label().text_color(m_active ? text_color : text_color_muted);
  } else if (state() == ButtonState::PRESSED) {
    sprite_pressed();
    label().text_color(m_active ? text_color : text_color_muted);
  } else if (state() == ButtonState::DISABLED) {
    sprite_disabled();
    label().text_color(default_settings::text_color_disabled);
  }
}

Tab& Tab::texture_active() {
  texture_disabled("tab_active_disabled")
    .texture_hovered("tab_active_hovered")
    .texture_idle("tab_active_idle")
    .texture_pressed("tab_active_pressed");
  label().text_color(theme::config().text_color);
  if (state() == ButtonState::DISABLED) {
    sprite_disabled();
  } else if (is_mouse_hovering()) {
    sprite_hovered();
  } else {
    sprite_idle();
  }
  return *this;
}

Tab& Tab::texture_inactive() {
  texture_disabled("tab_inactive_disabled")
    .texture_hovered("tab_inactive_hovered")
    .texture_idle("tab_inactive_idle")
    .texture_pressed("tab_inactive_pressed");
  label().text_color(theme::config().text_color_muted);
  if (state() == ButtonState::DISABLED) {
    sprite_disabled();
  } else if (is_mouse_hovering()) {
    sprite_hovered();
  } else {
    sprite_idle();
  }
  return *this;
}

void Tab::move_smooth(i32 to) {
  if (m_just_added) {
    move(to);
    m_just_added = false;
    return;
  }

  if (to != m_x_new) {
    m_x_old = x();
    m_x_new = to;
    m_t = 0.0f;
  }
}

void Tab::move(i32 to) {
  m_x_old = to;
  m_x_new = to;
  m_t = 1.0f;
}

Tab& Tab::active(bool a) {
  m_active = a;
  const auto& text_color = theme::config().text_color;
  const auto& text_color_muted = theme::config().text_color_muted;
  label().text_color(m_active ? text_color : text_color_muted);
  return *this;
}

TabBar::TabBar(Root& ui_) : Widget(ui_) {
  layout("ltr expand fill");
  // clip_children(true);

  m_tab_container = &add_child<Widget>();
  m_tab_container->clip_children(true);

  auto& pad = add_child<Widget>();
  pad.fixed_width(6);

  m_button_add = &add_child<ZincboxButton>("add_tab", theme::config().panel_top.button_add_tab);
  m_button_add->add_image("add_tab_icon");
  m_button_add->fixed_size(m_button_add->size());
  m_button_add->on_press([this]() {
    if (m_on_add_tab_button_pressed) { m_on_add_tab_button_pressed(); };
  });

  const float SCALE = zincbox::ui_scale();

  m_button_right = &add_child<ZincboxButton>("tab_right");
  m_button_right->add_image("right", true)
    .ignore_parents_layout(true)
    .x(-2 * SCALE)
    .parent_anchor(Anchor::CENTER_LEFT)
    .anchor(Anchor::CENTER_RIGHT);

  m_button_left = &add_child<ZincboxButton>("tab_left");
  m_button_left->add_image("left", true)
    .ignore_parents_layout(true)
    .size(m_button_left->image()->size())
    .x(2 * SCALE)
    .parent_anchor(Anchor::CENTER_LEFT)
    .anchor(Anchor::CENTER_LEFT);
}

void TabBar::update() {
  m_target_scroll_px = std::clamp<double>(m_target_scroll_px, 0, max_scroll_px());
  auto scroll_px_ = m_scroll_px;
  double delta = std::abs(m_scroll_px - m_target_scroll_px);
  if (delta > 2.0f) {
    double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
    m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);
  } else {
    m_scroll_px = scroll_px_;
  }
  if (Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT)) { m_dragged_tab_index = -1; }

  i32 mouse_x = Input::get_mouse_x() - x() + m_scroll_px;
  i32 mouse_drag_delta = std::abs(m_drag_start_mouse_pos - Input::get_mouse_x());

  if (m_dragged_tab_index != -1 && mouse_drag_delta > 10) {
    if (m_dragged_tab_index < (i32)m_tabs.size() - 1) {
      i32 target_idx = -1;
      for (i32 i = m_dragged_tab_index + 1; i < (i32)m_tabs.size(); i += 1) {
        if (m_tabs[i]->is_draggable()) {
          target_idx = i;
          break;
        }
      }

      if (target_idx != -1) {
        i32 sum = 0;
        for (i32 i = 0; i <= target_idx; i += 1) {
          sum += m_tabs[i]->width();
        }
        if (mouse_x > sum - m_tabs[target_idx]->width()) {
          if (swap_tabs(m_dragged_tab_index, target_idx)) { m_dragged_tab_index = target_idx; }
        }
      }
    }

    if (m_dragged_tab_index > 0) {
      i32 target_idx = -1;
      for (i32 i = m_dragged_tab_index - 1; i >= 0; i -= 1) {
        if (m_tabs[i]->is_draggable()) {
          target_idx = i;
          break;
        }
      }

      if (target_idx != -1) {
        i32 sum = 0;
        for (i32 i = 0; i < target_idx; i += 1) {
          sum += m_tabs[i]->width();
        }
        if (mouse_x < sum + m_tabs[target_idx]->width()) {
          if (swap_tabs(m_dragged_tab_index, target_idx)) { m_dragged_tab_index = target_idx; }
        }
      }
    }
  }

  position_tabs(true);

  m_button_right->x(m_tab_container->width());
  m_button_left->is_drawn(m_target_scroll_px > 0).is_updated(m_button_left->is_drawn());
  m_button_right->is_drawn(m_target_scroll_px < max_scroll_px()).is_updated(m_button_right->is_drawn());

  if (m_button_right->is_mouse_hovering() && m_button_right->is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    scroll(3);
  }

  if (m_button_left->is_mouse_hovering() && m_button_left->is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    scroll(-3);
  }

  m_scroll_px = std::clamp<double>(m_scroll_px, 0, max_scroll_px());

  const float SCALE = zincbox::ui_scale();
  m_button_right->size(height() - (4 * SCALE), height() - (4 * SCALE));
  m_button_left->size(height() - (4 * SCALE), height() - (4 * SCALE));

  Widget::update();
}

void TabBar::event(Input::InputEventMouseScroll& ev) {
  if (is_mouse_hovering()) {
    scroll(ev.offset.y * 15.0f);
    ev.handled = true;
  }
  if (!ev.handled) { Widget::event(ev); }
}

void TabBar::add_tab(const tab_info& info, bool select) { add_tab(info, m_tabs.size(), select); }

void TabBar::add_tab(const tab_info& info, size_t at, bool select) {
  at = std::min(at, m_tabs.size());
  Tab* t = &m_tab_container->add_child<Tab>(info);
  t->fixed_height(height());
  t->on_drag_start([this](i32 id) -> void { on_tab_drag_start(id); });
  t->on_active([this, info](Tab* tab) -> void {
    i32 mouse_drag_delta = std::abs(m_drag_start_mouse_pos - Input::get_mouse_x());
    if ((i32)tab->index() == m_dragged_tab_index && mouse_drag_delta > 10) { return; }

    update_tab_textures(tab->index());

    for (Tab* t_ : m_tabs) {
      t_->active(false);
    }

    tab->active(true);

    if (info.on_open) { info.on_open(); }
    if (m_on_tab_pressed) { m_on_tab_pressed(info.id); }
  });
  t->on_right_click(info.on_right_click);

  m_tabs.emplace(m_tabs.begin() + at, t);
  t->index(at).id(info.id);

  for (size_t i = t->index(); i < m_tabs.size(); i += 1) {
    m_tabs[i]->index(i);
  }

  if ((i32)at <= m_selected_tab_index) { m_selected_tab_index += 1; }

  if (select) {
    update_tab_textures(t->index());
    if (info.on_open) { info.on_open(); }
    if (m_on_tab_pressed) { m_on_tab_pressed(info.id); }
  }
}

void TabBar::select_tab(i32 id) {
  if (tab_valid(m_selected_tab_index) && m_tabs[m_selected_tab_index]->id() == id) { return; }
  auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [id](Tab* t) { return t->id() == id; });
  if (it != m_tabs.end()) {
    Tab* t = *it;
    t->trigger_active();
  }
}

void TabBar::unselect_all_tabs() {
  for (Tab* t : m_tabs) {
    t->active(false);
  }
  m_selected_tab_index = -1;
}

void TabBar::close_tab(i32 id) {
  auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [id](Tab* t) { return t->id() == id; });
  if (it != m_tabs.end()) {
    size_t removed_index = (*it)->index();
    (*it)->is_marked_for_deletion(true);
    m_tabs.erase(it);

    for (size_t i = 0; i < m_tabs.size(); i += 1) {
      m_tabs[i]->index(i);
    }

    if (m_selected_tab_index == (i32)removed_index) {
      m_selected_tab_index = -1;
    } else if (m_selected_tab_index > (i32)removed_index) {
      m_selected_tab_index -= 1;
    }
  }
}

void TabBar::close_all_tabs() {
  for (auto& t : m_tab_container->children()) {
    t->is_marked_for_deletion(true);
  }
  m_tabs.clear();
  m_selected_tab_index = -1;
}

void TabBar::update_tab_textures(i32 id) {
  for (Tab* t : m_tabs) {
    t->texture_inactive().draw_behind_parent(true);
  }
  m_selected_tab_index = id;
  if (tab_valid(m_selected_tab_index)) { m_tabs[m_selected_tab_index]->texture_active().draw_behind_parent(false); }
}

void TabBar::skip_anim() { position_tabs(false); }

void TabBar::sort_tabs_by_label(std::span<const std::string> labels) {
  auto* sel_tab = selected_tab();

  std::unordered_map<std::string, i32> label_priority;
  for (size_t i = 0; i < labels.size(); i += 1) {
    label_priority[labels[i]] = i;
  }

  std::sort(m_tabs.begin(), m_tabs.end(), [&](Tab* a, Tab* b) -> bool {
    auto it_lhs = label_priority.find(a->label().text());
    auto it_rhs = label_priority.find(b->label().text());
    if (it_lhs == label_priority.end()) { return false; }
    if (it_rhs == label_priority.end()) { return false; }
    return it_lhs->second < it_rhs->second;
  });

  m_selected_tab_index = -1;
  for (size_t i = 0; i < m_tabs.size(); i += 1) {
    m_tabs[i]->index(i);
    if (m_tabs[i] == sel_tab) { m_selected_tab_index = i; }
  }

  skip_anim();
}

void TabBar::scroll(double value) {
  value = std::clamp<double>(m_target_scroll_px + value, 0, max_scroll_px());
  if ((i32)m_target_scroll_px != (i32)value) { m_target_scroll_px = value; }
}

const Tab* TabBar::tab_by_label(const std::string& label) const {
  for (const Tab* tab : m_tabs) {
    if (tab->label().text() == label) { return tab; }
  }
  return nullptr;
}

i32 TabBar::tab_container_width() { return m_tab_container->max_width(); }

double TabBar::max_scroll_px() { return std::max(0, m_tab_container->max_width() - m_tab_container->width()); }

void TabBar::position_tabs(bool smooth) {
  i32 tab_x = 0;
  for (i32 id = 0; id < (i32)m_tabs.size(); id += 1) {
    Tab* tab = m_tabs[id];
    if (id != m_dragged_tab_index) {
      tab->is_drawn_on_top(false);
      if (smooth) {
        tab->move_smooth(tab_x - m_scroll_px);
      } else {
        tab->move(tab_x - m_scroll_px);
      }
    } else {
      tab->is_drawn_on_top(true);
      i32 dragged_tab_x = m_drag_start_tab_pos + Input::get_mouse_x() - m_drag_start_mouse_pos;
      tab->move(dragged_tab_x);
    }

    tab_x += tab->width();
  }
  m_tab_container->max_width(tab_x);
  m_content_size = tab_x + 6 + m_button_add->width();
}

void TabBar::on_tab_drag_start(i32 id) {
  if (tab_valid(id) && m_tabs[id]->is_draggable()) {
    m_dragged_tab_index = id;
    m_drag_start_mouse_pos = Input::get_mouse_x();
    m_drag_start_tab_pos = m_tabs[id]->x();
  }
}

bool TabBar::tab_valid(size_t index) const { return index < m_tabs.size(); }

bool TabBar::swap_tabs(size_t index_a, size_t index_b) {
  if (!tab_valid(index_a) || !tab_valid(index_b)) { return false; }
  if (!m_tabs[index_a]->is_draggable() || !m_tabs[index_b]->is_draggable()) { return false; }

  std::swap(m_tabs[index_a], m_tabs[index_b]);
  size_t idx_a = m_tabs[index_a]->index();
  m_tabs[index_a]->index(m_tabs[index_b]->index());
  m_tabs[index_b]->index(idx_a);

  if (m_selected_tab_index == (i32)index_a) {
    m_selected_tab_index = index_b;
  } else if (m_selected_tab_index == (i32)index_b) {
    m_selected_tab_index = index_a;
  }
  return true;
}
