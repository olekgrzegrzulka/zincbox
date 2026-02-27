#include <cstddef>
#include <memory>
#include "interface.hpp"
#include "panel_albums.hpp"
#include "panel_controls.hpp"
#include "panel_top.hpp"
#include "panel_tracks.hpp"
#include "popup_controller.hpp"
#include "texture_atlas.hpp"
#include "types.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

std::optional<musicdb::collection_id_t> active_collection_id;

std::unique_ptr<UI> ui;
PopupController* popup_controller{};
PanelTop* panel_top{};
Panel* panel_main{};
PanelTracks* panel_tracks{};
PanelAlbums* panel_albums{};
PanelControls* panel_controls{};

void init_atlas();

void interface::init() {
  ui = std::make_unique<UI>(1, 1);
  popup_controller = &ui->add_widget<PopupController>();
  popup_controller->set_is_drawn_on_top(true);

  init_atlas();

  panel_controls = &ui->add_widget<PanelControls>();
  panel_top = &ui->add_widget<PanelTop>();
  panel_main = &ui->add_widget<Panel>();
  panel_main->set_layout("m:0 s:0 ltr expand fill");
  panel_tracks = &panel_main->add_child<PanelTracks>();
  panel_albums = &panel_main->add_child<PanelAlbums>();

  panel_top->on_collection_opened = [&](musicdb::collection_id_t collection_id) {
    active_collection_id = collection_id;
    panel_tracks->recreate(active_collection_id);
    panel_albums->recreate(active_collection_id);
  };

  panel_albums->on_album_clicked = [&](size_t album_index_sorted) {
    panel_tracks->scroll_to_album(album_index_sorted);
  };

  if (std::filesystem::exists("musicdb")) {
    std::ifstream is("musicdb", std::ios::binary);
    musicdb::load_collections_from_file(is);
  }

  if (auto& c = musicdb::get_collections(); c.size() > 0) {
    panel_tracks->recreate(0);
    panel_albums->recreate(0);
    panel_top->recreate();
  }
}

void interface::process_input() {
  ui->process_input();
}

void interface::update(vec2i window_size) {
  panel_top->set_width(window_size.x);
  panel_main->set_y(panel_top->get_height());
  panel_main->set_width(window_size.x);
  panel_main->set_height(window_size.y - panel_top->get_height() - panel_controls->get_height());
  panel_controls->set_width(window_size.x);

  ui->update(window_size.x, window_size.y);
}

void interface::draw() {
  ui->draw();
}

void interface::deinit() {
  ui = nullptr;
}

PopupController* interface::get_popup_controller() { return popup_controller; }

void init_atlas() {
  auto& atlas = ui->get_texture_atlas();
  // ui
  atlas.add_texture("button_disabled", "./assets/button_disabled.png");
  atlas.add_texture("button_hovered", "./assets/button_hovered.png");
  atlas.add_texture("button_idle", "./assets/button_idle.png");
  atlas.add_texture("button_pressed", "./assets/button_pressed.png");
  atlas.add_texture("combo_box_button_contract", "./assets/combo_box_button_contract.png");
  atlas.add_texture("combo_box_button_expand", "./assets/combo_box_button_expand.png");
  atlas.add_texture("combo_box", "./assets/combo_box.png");
  atlas.add_texture("dim", "./assets/dim.png");
  atlas.add_texture("panel_rectangular_highlighted", "./assets/panel_rectangular_highlighted.png");
  atlas.add_texture("panel_rectangular", "./assets/panel_rectangular.png");
  atlas.add_texture("panel_rectangular_dark", "./assets/panel_rectangular_dark.png");
  atlas.add_texture("panel_rounded_dark", "./assets/panel_rounded_dark.png");
  atlas.add_texture("panel_rounded_light", "./assets/panel_rounded_light.png");
  atlas.add_texture("panel_rectangular_light", "./assets/panel_rectangular_light.png");
  atlas.add_texture("panel_rounded", "./assets/panel_rounded.png");
  atlas.add_texture("panel_shadow", "./assets/panel_shadow.png");
  atlas.add_texture("red", "./assets/red.png");
  atlas.add_texture("selectbar_bg", "./assets/selectbar_bg.png");
  atlas.add_texture("selectbar_selected", "./assets/selectbar_selected.png");
  atlas.add_texture("slider_thumb_hovered", "./assets/slider_thumb_hovered.png");
  atlas.add_texture("slider_thumb_idle", "./assets/slider_thumb_idle.png");
  atlas.add_texture("slider_thumb_pressed", "./assets/slider_thumb_pressed.png");
  atlas.add_texture("slider_track", "./assets/slider_track.png");
  atlas.add_texture("scrollbar_thumb_hovered", "./assets/scrollbar_thumb_hovered.png");
  atlas.add_texture("scrollbar_thumb_idle", "./assets/scrollbar_thumb_idle.png");
  atlas.add_texture("scrollbar_thumb_pressed", "./assets/scrollbar_thumb_pressed.png");
  atlas.add_texture("scrollbar_track", "./assets/scrollbar_track.png");
  atlas.add_texture("spinner_buttons", "./assets/spinner_buttons.png");
  atlas.add_texture("text_input_caret", "./assets/text_input_caret.png");
  atlas.add_texture("text_input_focused", "./assets/text_input_focused.png");
  atlas.add_texture("text_input_idle", "./assets/text_input_idle.png");
  // player
  atlas.add_texture("seekbar_bg", "./assets/seekbar_bg.png");
  atlas.add_texture("seekbar_progress", "./assets/seekbar_progress.png");
  atlas.add_texture("seekbar_thumb", "./assets/seekbar_thumb.png");
  atlas.add_texture("track_bg1", "./assets/track_bg1.png");
  atlas.add_texture("track_bg2", "./assets/track_bg2.png");
  atlas.add_texture("track_bg_playing", "./assets/track_bg_playing.png");
  atlas.add_texture("tab_active_disabled", "./assets/tab_active_disabled.png");
  atlas.add_texture("tab_active_hovered", "./assets/tab_active_hovered.png");
  atlas.add_texture("tab_active_idle", "./assets/tab_active_idle.png");
  atlas.add_texture("tab_active_pressed", "./assets/tab_active_pressed.png");
  atlas.add_texture("tab_inactive_disabled", "./assets/tab_inactive_disabled.png");
  atlas.add_texture("tab_inactive_hovered", "./assets/tab_inactive_hovered.png");
  atlas.add_texture("tab_inactive_idle", "./assets/tab_inactive_idle.png");
  atlas.add_texture("tab_inactive_pressed", "./assets/tab_inactive_pressed.png");
  atlas.add_texture("popover_panel", "./assets/popover_panel.png");
  atlas.add_texture("popover_arrow", "./assets/popover_arrow.png");
  // icons
  atlas.add_texture("play", "./assets/icons/play.png");
  atlas.add_texture("pause", "./assets/icons/pause.png");
  atlas.add_texture("stop", "./assets/icons/stop.png");
  atlas.add_texture("next", "./assets/icons/next.png");
  atlas.add_texture("prev", "./assets/icons/prev.png");
  atlas.add_texture("repeat", "./assets/icons/repeat.png");
  atlas.add_texture("repeat_off", "./assets/icons/repeat_off.png");
  atlas.add_texture("repeat_album", "./assets/icons/repeat_album.png");
  atlas.add_texture("repeat_track", "./assets/icons/repeat_track.png");
  atlas.add_texture("shuffle", "./assets/icons/shuffle.png");
  atlas.add_texture("shuffle_off", "./assets/icons/shuffle_off.png");
  atlas.save_to_file("atlas.png");
}

void handle_dropped_files() {
  std::vector<std::string> dropped_directories{};
  for (auto& path : Input::get_dropped_paths()) {
    if (std::filesystem::is_directory(path)) {
      dropped_directories.emplace_back(path);
    }
  }
  std::string content;
  i32 longest_path_length = 0;
  i32 index = 1;
  for (auto& dir : dropped_directories) {
    content += std::to_string(index) + ". " + dir + "\n";
    longest_path_length = std::max(longest_path_length, (i32)dir.length());
    index += 1;
  }

  if (dropped_directories.size() > 0) {
    popup_descriptor d{
      .id = "a",
      .title = "Do you want to create the following collections?",
      .content = content,
      .button_labels = {"Cancel", "Add"},
      .button_actions = {[] {}, [] {}},
    };
    interface::get_popup_controller()->create_popup(d);
  }
}
