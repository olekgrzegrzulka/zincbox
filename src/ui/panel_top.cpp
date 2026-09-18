#include "panel_top.hpp"
#include <algorithm>
#include <string>
#include "common/color.hpp"
#include "common/types.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/zincbox.hpp"
#include "theme_config.hpp"
#include "tr.hpp"
#include "ui/tab_bar.hpp"
#include "ui/theme.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/color_rect.hpp"
#include "ui_generic/widget.hpp"

class UI;

static constexpr size_t QUEUE_TAB_ID = 10000;

PanelTop::PanelTop(UI& ui_) : ColorRect(ui_) {
  static const float scale = zincbox::ui_scale();
  set_color(theme::config().top_bar.color);
  set_height(theme::config().top_bar.height * scale);

  container_tabbar = &add_child<Widget>();
  container_tabbar->set_parent_anchor(Anchor::TOP_LEFT);
  container_tabbar->set_anchor(Anchor::TOP_LEFT);
  // container_tabbar->set_clip_children(true);
  container_tabbar->set_layout("expand fill");

  container_drag_region = &add_child<Widget>();
  container_drag_region->set_parent_anchor(Anchor::TOP_LEFT);
  container_drag_region->set_anchor(Anchor::TOP_LEFT);
  container_drag_region->set_is_self_drawn(false);
  container_drag_region->set_min_width(100 * scale);
  container_drag_region->set_max_width(9999 * scale);

  container_buttons = &add_child<Widget>();
  container_buttons->set_layout("rtl expand fit");
  container_buttons->get_layout().margin = {scale, scale};
  container_buttons->get_layout().spacing = scale;
  container_buttons->set_parent_anchor(Anchor::TOP_RIGHT);
  container_buttons->set_anchor(Anchor::TOP_RIGHT);

  tab_bar = &container_tabbar->add_child<TabBar>();
  tab_bar->set_height(height);
  tab_bar->get_button_add()->set_width(height - 1 * 7 * scale);
  tab_bar->get_button_add()->set_height(height - 1 * 7 * scale);

  button_decor_close = &container_buttons->add_child<ZincboxButton>("button_decor_close");
  button_decor_close->add_image("icon_decor_close");
  button_decor_close->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                   theme::config().custom_window_decoration.show_close_button);
  button_decor_close->on_press([this]() -> void {
    if (on_close_button_pressed) { on_close_button_pressed(); }
  });

  button_decor_maximize = &container_buttons->add_child<ZincboxButton>("button_decor_maximize");
  button_decor_maximize->add_image("icon_decor_maximize");
  button_decor_maximize->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                      theme::config().custom_window_decoration.show_maximize_button);
  button_decor_maximize->on_press([this]() -> void {
    if (on_maximize_button_pressed) { on_maximize_button_pressed(); }
  });

  button_decor_minimize = &container_buttons->add_child<ZincboxButton>("button_decor_minimize");
  button_decor_minimize->add_image("icon_decor_minimize");
  button_decor_minimize->set_is_drawn(theme::config().custom_window_decoration.enabled &&
                                      theme::config().custom_window_decoration.show_minimize_button);
  button_decor_minimize->on_press([this]() -> void {
    if (on_minimize_button_pressed) { on_minimize_button_pressed(); }
  });

  button_hamburger = &container_buttons->add_child<Button>("");
  button_hamburger->add_image("hamburger");
  button_hamburger->set_size(height - 4 * scale, height - 4 * scale);
  button_hamburger->on_press([this]() {
    if (this->on_hamburger_button_pressed) { this->on_hamburger_button_pressed(this->button_hamburger); }
  });
}

const Tab* PanelTop::get_queue_tab() const {
  for (auto& tab : tab_bar->get_tabs()) {
    if (tab->id == QUEUE_TAB_ID) { return tab; }
  }
  return nullptr;
}

void PanelTop::update() {
  container_tabbar->set_width(std::min(tab_bar->get_tab_container_width(),
                                       width - 100 - container_buttons->get_width())); // FIXME
  container_drag_region->set_x(container_tabbar->get_width());
  container_drag_region->set_width(width - container_tabbar->get_width() - container_buttons->get_width());

  container_tabbar->set_height(height);
  container_drag_region->set_height(height);
  container_buttons->set_height(height);

  tab_bar->set_x(std::clamp(tab_bar->get_x(), container_tabbar->get_width() - tab_bar->get_tab_container_width(), 0));
  ColorRect::update();
}

void PanelTop::recreate(std::optional<size_t> selected_collection_id) {
  tab_bar->close_all_tabs();
  tab_bar->on_add_tab_button_pressed = [this]() {
    if (this->on_add_collection_button_pressed) { this->on_add_collection_button_pressed(tab_bar); }
  };

  tab_bar->add_tab(TabBar::tab_info{
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

    tab_bar->add_tab(TabBar::tab_info{
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
}

void PanelTop::select(size_t selected_collection_id) { tab_bar->select_tab(selected_collection_id); }

bool PanelTop::can_drag_window() { return container_drag_region->is_mouse_hovering(); }
