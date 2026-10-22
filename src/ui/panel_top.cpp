#include "panel_top.hpp"
#include <algorithm>
#include <string>
#include "common/color.hpp"
#include "common/types.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "ui/tab_bar.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

static constexpr size_t QUEUE_TAB_ID = 10000;

PanelTop::PanelTop(zincgui::Root& ui_) : ColorRect(ui_) {
  const float SCALE = zincbox::ui_scale();
  set_color(theme::config().panel_top.color);
  set_height(theme::config().panel_top.height * SCALE);

  m_container_tabbar = &add_child<Widget>();
  m_container_tabbar->set_parent_anchor(zincgui::Anchor::TOP_LEFT);
  m_container_tabbar->set_anchor(zincgui::Anchor::TOP_LEFT);
  // m_container_tabbar->set_clip_children(true);
  m_container_tabbar->set_layout("expand fill");

  m_container_drag_region = &add_child<Widget>();
  m_container_drag_region->set_parent_anchor(zincgui::Anchor::TOP_LEFT);
  m_container_drag_region->set_anchor(zincgui::Anchor::TOP_LEFT);
  m_container_drag_region->set_is_self_drawn(false);
  m_container_drag_region->set_min_width(100 * SCALE);
  m_container_drag_region->set_max_width(9999 * SCALE);

  m_container_buttons = &add_child<Widget>();
  m_container_buttons->set_layout("rtl expand fit");
  m_container_buttons->get_layout().margin = {SCALE, SCALE};
  m_container_buttons->get_layout().spacing = SCALE;
  m_container_buttons->set_parent_anchor(zincgui::Anchor::TOP_RIGHT);
  m_container_buttons->set_anchor(zincgui::Anchor::TOP_RIGHT);

  m_tab_bar = &m_container_tabbar->add_child<TabBar>();
  m_tab_bar->set_height(height);
  m_tab_bar->get_button_add()->set_width(height - (1 * 7 * SCALE));
  m_tab_bar->get_button_add()->set_height(height - (1 * 7 * SCALE));

  m_button_decor_close = &m_container_buttons->add_child<ZincboxButton>("button_decor_close");
  m_button_decor_close->add_image("icon_decor_close");
  m_button_decor_close->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                     theme::config().custom_window_decoration.show_close_button);
  m_button_decor_close->on_press([this]() -> void {
    if (on_close_button_pressed) { on_close_button_pressed(); }
  });

  m_button_decor_maximize = &m_container_buttons->add_child<ZincboxButton>("button_decor_maximize");
  m_button_decor_maximize->add_image("icon_decor_maximize");
  m_button_decor_maximize->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                        theme::config().custom_window_decoration.show_maximize_button);
  m_button_decor_maximize->on_press([this]() -> void {
    if (on_maximize_button_pressed) { on_maximize_button_pressed(); }
  });

  m_button_decor_minimize = &m_container_buttons->add_child<ZincboxButton>("button_decor_minimize");
  m_button_decor_minimize->add_image("icon_decor_minimize");
  m_button_decor_minimize->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                        theme::config().custom_window_decoration.show_minimize_button);
  m_button_decor_minimize->on_press([this]() -> void {
    if (on_minimize_button_pressed) { on_minimize_button_pressed(); }
  });

  m_button_hamburger = &m_container_buttons->add_child<zincgui::Button>("");
  m_button_hamburger->add_image("hamburger");
  m_button_hamburger->set_size(height - (4 * SCALE), height - (4 * SCALE));
  m_button_hamburger->on_press([this]() {
    if (this->on_hamburger_button_pressed) { this->on_hamburger_button_pressed(this->m_button_hamburger); }
  });
}

const Tab* PanelTop::get_queue_tab() const {
  for (auto& tab : m_tab_bar->get_tabs()) {
    if (tab->id == QUEUE_TAB_ID) { return tab; }
  }
  return nullptr;
}

void PanelTop::update() {
  m_container_tabbar->set_width(std::min(m_tab_bar->get_tab_container_width(),
                                         width - 100 - m_container_buttons->get_width())); // FIXME
  m_container_drag_region->set_x(m_container_tabbar->get_width());
  m_container_drag_region->set_width(width - m_container_tabbar->get_width() - m_container_buttons->get_width());

  m_container_tabbar->set_height(height);
  m_container_drag_region->set_height(height);
  m_container_buttons->set_height(height);

  m_tab_bar->set_x(std::clamp(m_tab_bar->get_x(),
                              std::min(0, m_container_tabbar->get_width() - m_tab_bar->get_tab_container_width()), 0));
  ColorRect::update();
}

void PanelTop::recreate(std::optional<size_t> selected_collection_id) {
  m_tab_bar->close_all_tabs();
  m_tab_bar->on_add_tab_button_pressed = [this]() {
    if (this->on_add_collection_button_pressed) { this->on_add_collection_button_pressed(m_tab_bar); }
  };

  m_tab_bar->add_tab(tab_info{
                       .id = QUEUE_TAB_ID,
                       .is_draggable = false,
                       .label = tr::get("tab.queue"),
                       .padding = 10,
                       .on_open =
                         [this]() {
                           if (on_queue_view_opened) { on_queue_view_opened(); }
                         },
                       .on_right_click =
                         [this](Tab* t) {
                           if (this->on_queue_rmb) { this->on_queue_rmb(t); }
                         },
                     },
                     0, false);

  for (size_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    auto& collection = db::collection_by_id(collection_id)->get();
    if (collection.is_tombstone()) { continue; }

    m_tab_bar->add_tab(tab_info{
                         .id = (i32)collection_id,
                         .is_draggable = true,
                         .label = std::string(collection.name()),
                         .padding = 20,
                         .on_open =
                           [this, collection_id]() {
                             if (on_collection_opened) { this->on_collection_opened(collection_id); }
                           },
                         .on_right_click =
                           [this, collection_id](Tab* t) {
                             if (this->on_show_collection_actions_popover) {
                               this->on_show_collection_actions_popover(collection_id, t);
                             }
                           },
                       },
                       1000, collection_id == selected_collection_id);
  }
  m_tab_bar->skip_anim();
}

void PanelTop::select(size_t selected_collection_id) { m_tab_bar->select_tab(selected_collection_id); }

bool PanelTop::can_drag_window() { return m_container_drag_region->is_mouse_hovering(); }
