#pragma once
#include <functional>
#include "musicdb.hpp"
#include "tab_bar.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

class PanelTop : public Panel {
public:
  PanelTop(UI& ui_) : Panel(ui_), tab_bar(add_child<TabBar>()) {
    set_height(30);

    auto* btn_settings = &add_child<Button>("");
    btn_settings->set_size(26, 26);
    btn_settings->set_x(-2);
    btn_settings->set_y(2);
    btn_settings->set_parent_anchor(Anchor::TOP_RIGHT);
    btn_settings->set_anchor(Anchor::TOP_RIGHT);
    auto* btn_settings_img = &btn_settings->add_child<Sprite>("settings");
    btn_settings_img->set_anchor(Anchor::CENTER);
    btn_settings_img->set_parent_anchor(Anchor::CENTER);
  }

  void update() override {
    set_width(ui.get_window_width());
  }

  void recreate(musicdb::collection_id_t collection_id) {
    tab_bar.close_all_tabs();

    tab_bar.add_tab(TabBar::tab_info{
                      .is_draggable = false,
                      .label = "Queue",
                      .on_open = [this]() {
                        if (on_queue_view_opened) {
                          on_queue_view_opened();
                        }
                      },
                      .padding = 10,
                    },
                    0, false);

    tab_bar.add_tab(TabBar::tab_info{
                      .is_draggable = false,
                      .label = "Playlists",
                      .on_open = [this]() {
                        if (on_playlists_view_opened) {
                          on_playlists_view_opened();
                        }
                      },
                      .padding = 10,
                    },
                    1, false);

    for (auto c : musicdb::get_collections()) {
      auto c_id = c.get_id();
      tab_bar.add_tab(
        TabBar::tab_info{
          .id = (i32)c.get_id(),
          .is_draggable = true,
          .label = c.get_name(),
          .on_open = [this, c_id]() {
            if (on_collection_opened) { on_collection_opened(c_id); }
          },
          .padding = 20,
        },
        1000, c.get_id() == collection_id);
    }
  }

public:
  std::function<void(musicdb::collection_id_t)> on_collection_opened{};
  std::function<void()> on_playlists_view_opened{};
  std::function<void()> on_queue_view_opened{};

protected:
  TabBar& tab_bar;
};
