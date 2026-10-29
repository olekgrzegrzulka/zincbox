#include "panel_top.hpp"
#include <algorithm>
#include <string>
#include <utility>
#include "common/color.hpp"
#include "common/logger.hpp"
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
  color(theme::config().panel_top.color)
    .height(theme::config().panel_top.height * SCALE);

  m_container_tabbar = &add_child<Widget>();
  m_container_tabbar->layout("expand fill");

  m_container_drag_region = &add_child<Widget>();
  m_container_drag_region->hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH);

  m_container_buttons = &add_child<Widget>();
  m_container_buttons->layout("rtl expand fit");
  m_container_buttons->layout().margin = {SCALE, SCALE};
  m_container_buttons->layout().spacing = SCALE;

  m_tab_bar = &m_container_tabbar->add_child<TabBar>();
  m_tab_bar->height(height());
  m_tab_bar->button_add()->width(height() - (1 * 7 * SCALE))
    .height(height() - (1 * 7 * SCALE));

  m_button_decor_close = &m_container_buttons->add_child<ZincboxButton>("button_decor_close");
  m_button_decor_close->add_image("icon_decor_close");
  m_button_decor_close->fixed_height(0)
    .is_drawn(theme::config().custom_window_decoration.enabled &&
              theme::config().custom_window_decoration.show_close_button)
    .on_press([this]() -> void {
      if (m_on_close_button_pressed) { m_on_close_button_pressed(); }
    });

  m_button_decor_maximize = &m_container_buttons->add_child<ZincboxButton>("button_decor_maximize");
  m_button_decor_maximize->add_image("icon_decor_maximize");
  m_button_decor_maximize->fixed_height(0)
    .is_drawn(theme::config().custom_window_decoration.enabled &&
              theme::config().custom_window_decoration.show_maximize_button)
    .on_press([this]() -> void {
      if (m_on_maximize_button_pressed) { m_on_maximize_button_pressed(); }
    });

  m_button_decor_minimize = &m_container_buttons->add_child<ZincboxButton>("button_decor_minimize");
  m_button_decor_minimize->add_image("icon_decor_minimize");
  m_button_decor_minimize->fixed_height(0)
    .is_drawn(theme::config().custom_window_decoration.enabled &&
              theme::config().custom_window_decoration.show_minimize_button)
    .on_press([this]() -> void {
      if (m_on_minimize_button_pressed) { m_on_minimize_button_pressed(); }
    });

  m_button_hamburger = &m_container_buttons->add_child<zincgui::Button>("");
  m_button_hamburger->add_image("hamburger");
  m_button_hamburger->size(height() - (4 * SCALE), height() - (4 * SCALE))
    .fixed_height(0)
    .on_press([this]() {
      if (m_on_hamburger_button_pressed) { m_on_hamburger_button_pressed(m_button_hamburger); }
    });
}

void PanelTop::update() {
  if (m_width_old == width() && m_height_old == height()) {
    ColorRect::update();
    return;
  }

  m_width_old = width();
  m_height_old = height();

  const float SCALE = zincbox::ui_scale();
  const i32 DRAG_REGION_WIDTH = 100 * SCALE;

  if (width() >= m_tab_bar->content_size() + DRAG_REGION_WIDTH + m_container_buttons->width()) {
    m_container_tabbar->width(m_tab_bar->content_size());
    m_container_drag_region->width(width() - m_tab_bar->content_size() - m_container_buttons->width());
  } else {
    m_container_tabbar->width(width() - DRAG_REGION_WIDTH - m_container_buttons->width());
    m_container_drag_region->width(DRAG_REGION_WIDTH);
  }

  m_container_tabbar->height(height());
  m_container_drag_region->height(height());
  m_container_buttons->height(height());

  m_container_tabbar->x(0);
  m_container_drag_region->x(m_container_tabbar->width());
  m_container_buttons->x(m_container_tabbar->width() + m_container_drag_region->width());

  ColorRect::update();
}

void PanelTop::recreate(std::optional<size_t> selected_collection_id) {
  m_tab_bar->close_all_tabs();
  m_tab_bar->on_add_tab_button_pressed([this]() {
    if (m_on_add_collection_button_pressed) { m_on_add_collection_button_pressed(m_tab_bar); }
  });

  m_tab_bar->add_tab(tab_info{
                       .id = QUEUE_TAB_ID,
                       .is_draggable = false,
                       .label = tr::get("tab.queue"),
                       .padding = 10,
                       .on_open =
                         [this]() {
                           if (m_on_queue_view_opened) { m_on_queue_view_opened(); }
                         },
                       .on_right_click =
                         [this](Tab* t) {
                           if (m_on_queue_rmb) { m_on_queue_rmb(t); }
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
                             if (m_on_collection_opened) { m_on_collection_opened(collection_id); }
                           },
                         .on_right_click =
                           [this, collection_id](Tab* t) {
                             if (m_on_show_collection_actions_popover) {
                               m_on_show_collection_actions_popover(collection_id, t);
                             }
                           },
                       },
                       1000, collection_id == selected_collection_id);
  }
  m_tab_bar->skip_anim();
}

void PanelTop::select(size_t selected_collection_id) { m_tab_bar->select_tab(selected_collection_id); }

bool PanelTop::can_drag_window() { return m_container_drag_region->is_mouse_hovering(); }

const Tab* PanelTop::queue_tab() const {
  for (auto& tab : m_tab_bar->tabs()) {
    if (tab->id() == QUEUE_TAB_ID) { return tab; }
  }
  return nullptr;
}
