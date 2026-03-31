#include "panel_top.hpp"
#include "core/musicdb.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"

PanelTop::PanelTop(UI& ui_) : Panel(ui_), tab_bar(add_child<TabBar>()) {
  set_height(32);

  auto* btn_settings = &add_child<Button>("");
  btn_settings->set_size(26, 26);
  btn_settings->set_x(-2);
  btn_settings->set_y(2);
  btn_settings->set_parent_anchor(Anchor::TOP_RIGHT);
  btn_settings->set_anchor(Anchor::TOP_RIGHT);
  btn_settings->on_press([this, btn_settings]() {
    if (this->on_settings_button_pressed) {
      this->on_settings_button_pressed(btn_settings);
    }
  });

  auto* btn_settings_img = &btn_settings->add_child<Sprite>("settings");
  btn_settings_img->set_anchor(Anchor::CENTER);
  btn_settings_img->set_parent_anchor(Anchor::CENTER);
}

void PanelTop::update() {
  set_width(ui.get_window_width());
}

void PanelTop::recreate(std::optional<size_t> selected_collection_id) {
  tab_bar.close_all_tabs();
  tab_bar.on_add_tab_button_pressed = [this]() {
    if (this->on_add_collection_button_pressed) {
      this->on_add_collection_button_pressed(&tab_bar);
    }
  };

  tab_bar.add_tab(TabBar::tab_info{
                    .is_draggable = false,
                    .label = U"Queue",
                    .padding = 10,
                    .on_open = [this]() {
                      if (on_queue_view_opened) {
                        on_queue_view_opened();
                      }
                    },
                  },
                  0, false);

  for (size_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    auto& collection = db::collection_by_id(collection_id)->get();
    if (collection.is_tombstone()) { continue; }

    tab_bar.add_tab(
      TabBar::tab_info{
        .id = (i32)collection_id,
        .is_draggable = true,
        .label = collection.name,
        .padding = 20,
        .on_open = [this, collection_id]() {
          if (on_collection_opened) { this->on_collection_opened(collection_id); } },
        .on_right_click = [this, collection_id](Tab* t) {
          if (this->on_show_collection_actions_popover) {
            this->on_show_collection_actions_popover(collection_id, t->get_position(Anchor::BOTTOM));
          } },
      },
      1000, ((selected_collection_id.has_value()) && collection_id == selected_collection_id));
  }
}
