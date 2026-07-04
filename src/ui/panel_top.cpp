#include "panel_top.hpp"
#include <string>
#include "common/input.hpp"
#include "core/musicdb/musicdb.hpp"
#include "tr.hpp"
#include "ui/tab_bar.hpp"
#include "ui/theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"

static constexpr size_t QUEUE_TAB_ID = 10000;

PanelTop::PanelTop(UI& ui_) : Sprite(ui_, "panel_top") {
  set_height(theme::get_prop("top_bar_height").as_i32(32));

  tab_bar = &add_child<TabBar>();

  button_settings = &add_child<Button>("");
  button_settings->set_size(height - 4, height - 4);
  button_settings->set_x(-2);
  button_settings->set_parent_anchor(Anchor::CENTER_RIGHT);
  button_settings->set_anchor(Anchor::CENTER_RIGHT);
  button_settings->on_press([this]() {
    if (this->on_settings_button_pressed) { this->on_settings_button_pressed(this->button_settings); }
  });

  button_right = &add_child<Button>("");
  button_right->set_size(height - 4, height - 4);
  button_right->set_x(-2 - (height - 4) - 2);
  button_right->set_parent_anchor(Anchor::CENTER_RIGHT);
  button_right->set_anchor(Anchor::CENTER_RIGHT);

  button_left = &add_child<Button>("");
  button_left->set_size(height - 4, height - 4);
  button_left->set_x(2);
  button_left->set_parent_anchor(Anchor::CENTER_LEFT);
  button_left->set_anchor(Anchor::CENTER_LEFT);

  auto* btn_settings_img = &button_settings->add_child<Sprite>("settings");
  btn_settings_img->set_anchor(Anchor::CENTER);
  btn_settings_img->set_parent_anchor(Anchor::CENTER);

  auto& button_right_img = button_right->add_child<Sprite>("right");
  button_right_img.set_anchor(Anchor::CENTER);
  button_right_img.set_parent_anchor(Anchor::CENTER);

  auto& button_left_img = button_left->add_child<Sprite>("left");
  button_left_img.set_anchor(Anchor::CENTER);
  button_left_img.set_parent_anchor(Anchor::CENTER);
}

const Tab* PanelTop::get_queue_tab() const {
  for (auto& tab : tab_bar->get_tabs()) {
    if (tab->id == QUEUE_TAB_ID) { return tab; }
  }
  return nullptr;
}

void PanelTop::input() {
  std::array<Widget*, 4> custom_children_input_update_order = {button_right, button_left, button_settings, tab_bar};

  for (auto&& child : custom_children_input_update_order) {
    if (child->get_is_updated() && !child->get_marked_for_deletion()) { child->input(); }
  }

  for (auto& ev : Input::get_event_queue()) {
    std::visit([&](auto& ev) { Widget::event(ev); }, ev);
  }
}

void PanelTop::update() {
  set_width(ui.get_window_width());

  button_left->set_is_drawn(tab_bar->get_x() != 0);
  button_left->set_is_updated(button_left->get_is_drawn());
  button_right->set_is_drawn(ui.get_window_width() < tab_bar->get_width() &&
                             tab_bar->get_x() != ui.get_window_width() - tab_bar->get_width());
  button_right->set_is_updated(button_right->get_is_drawn());

  if (button_right->is_mouse_hovering() && button_right->get_is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    tab_bar->set_x(std::max(tab_bar->get_x() - 3, ui.get_window_width() - tab_bar->get_width()));
    ui.mark_dirty_recursive(this);
  }

  if (button_left->is_mouse_hovering() && button_left->get_is_drawn() &&
      Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT)) {
    tab_bar->set_x(std::min(tab_bar->get_x() + 3, 0));
    ui.mark_dirty_recursive(this);
  }

  Sprite::update();
}

void PanelTop::recreate(std::optional<size_t> selected_collection_id) {
  tab_bar->close_all_tabs();
  tab_bar->on_add_tab_button_pressed = [this]() {
    if (this->on_add_collection_button_pressed) { this->on_add_collection_button_pressed(tab_bar); }
  };

  tab_bar->add_tab(
    TabBar::tab_info{
      .id = QUEUE_TAB_ID,
      .is_draggable = false,
      .label = tr::get("tab.queue"),
      .padding = 10,
      .on_open =
        [this]() {
          if (on_queue_view_opened) { on_queue_view_opened(); }
        },
    },
    0, false);

  for (size_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    auto& collection = db::collection_by_id(collection_id)->get();
    if (collection.is_tombstone()) { continue; }

    tab_bar->add_tab(TabBar::tab_info{
                       .id = (i32)collection_id,
                       .is_draggable = true,
                       .label = std::u32string(collection.name()),
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
