#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include "common/debug.hpp"
#include "common/types.hpp"
#include "core/mpris.hpp"
#include "core/musicdb.hpp"
#include "core/player.hpp"
#include "core/utf.hpp"
#include "interface.hpp"
#include "panel_albums.hpp"
#include "panel_controls.hpp"
#include "panel_queue.hpp"
#include "panel_top.hpp"
#include "panel_tracks.hpp"
#include "popup_controller.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/panel.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"
#include "lib/nativefiledialog-extended/src/include/nfd.hpp"

// namespace fs = std::filesystem;

enum class InterfaceView {
  QUEUE,
  COLLECTION,
  PLAYLISTS,
};

static InterfaceView current_view = InterfaceView::COLLECTION;
static std::optional<size_t> active_collection_id;
static std::vector<std::unique_ptr<TextureAtlas>> album_cover_atlases;

static std::unique_ptr<UI> ui;
static PopupController* popup_controller{};
static PanelTop* panel_top{};
static Panel* panel_main{};
static PanelTracks* panel_tracks{};
static PanelQueue* panel_queue{};
static PanelAlbums* panel_albums{};
static PanelControls* panel_controls{};

void init_atlas();
void init_album_cover_atlases();
void handle_dropped_files();
void delete_collection(size_t);
void delete_playlist_track(size_t);
void show_add_to_playlist_popup(size_t);

void interface::init() {
  ui = std::make_unique<UI>(1, 1);
  init_atlas();
  init_album_cover_atlases();

  popup_controller = &ui->add_widget<PopupController>();
  popup_controller->set_is_drawn_on_top(true);

  panel_controls = &ui->add_widget<PanelControls>();
  panel_top = &ui->add_widget<PanelTop>();
  panel_main = &ui->add_widget<Panel>();
  panel_main->set_layout("m:0 s:0 ltr expand fill");
  panel_tracks = &panel_main->add_child<PanelTracks>();
  panel_queue = &panel_main->add_child<PanelQueue>();
  panel_queue->set_is_drawn(false);
  panel_albums = &panel_main->add_child<PanelAlbums>();

  panel_top->on_collection_opened = [&](size_t collection_id) {
    current_view = InterfaceView::COLLECTION;
    active_collection_id = collection_id;
    panel_tracks->view_type = PanelTracks::ViewType::COLLECTION;
    panel_tracks->collection_id = active_collection_id;
    panel_tracks->clear();
    panel_tracks->recreate();
    panel_tracks->set_is_drawn(true);
    panel_tracks->set_is_updated(true);
    panel_queue->set_is_drawn(false);
    panel_queue->set_is_updated(false);
    TextureAtlas* atlas = nullptr;
    if (active_collection_id.has_value()) { atlas = album_cover_atlases[*active_collection_id].get(); }
    panel_albums->recreate(active_collection_id, atlas);
  };

  panel_top->on_queue_view_opened = [&]() {
    current_view = InterfaceView::QUEUE;
    active_collection_id = std::nullopt;
    panel_tracks->set_is_drawn(false);
    panel_tracks->set_is_updated(false);
    panel_queue->set_is_drawn(true);
    panel_queue->set_is_updated(true);
    panel_queue->on_queue_changed();
    panel_albums->recreate(std::nullopt, nullptr);
  };

  panel_top->on_show_collection_actions_popover = [&](size_t collection_id, vec2i at) {
    popover_descriptor d{
      .id = "collection_actions",
      .at = at,
      .button_labels = {"Delete"},
      .button_actions = {[collection_id]() {
        delete_collection(collection_id);
      }},
    };
    popup_controller->create_popover(d);
  };

  panel_top->on_add_collection_button_pressed = [&](Widget*) {
    NFD::UniquePathSet out_paths;
    auto result = NFD::PickFolderMultiple(out_paths, nullptr);
    if (result == NFD_OKAY) {
      nfdpathsetsize_t numPaths;
      NFD::PathSet::Count(out_paths, numPaths);
      if (numPaths > 0) {
        nfdpathsetsize_t i;
        for (i = 0; i < numPaths; ++i) {
          NFD::UniquePathSetPath path;
          NFD::PathSet::GetPath(out_paths, i, path);
          std::cout << "Path " << i << ": " << path.get() << std::endl;

          std::string collection_name = "Collection #" + std::to_string(db::collection_count() + 1);
          auto collection_id = db::add_collection(utf8_to_utf32(collection_name));
          auto& collection = db::collection_by_id(collection_id)->get();
          collection.add_path(path.get());
        }
      }
    }
  };

  panel_tracks->on_track_lmb = [&](size_t collection_id, size_t playlist_id, size_t track_id, size_t /* playlist_track_index */, Widget*) {
    player::playing_t play{
      .collection_id = collection_id,
      .playlist_id = playlist_id,
      .track_id = track_id,
    };
    player::play(play, true);
  };

  panel_queue->on_queue_element_lmb = [&](size_t queue_index, Widget*) {
    player::set_playing_index(queue_index);
  };

  panel_queue->on_queue_element_rmb = [&](size_t, Widget*) {
  };

  panel_tracks->on_track_rmb = [&](size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget) {
    size_t loved_tracks_playlist_id = db::collection_by_id(0)->get().playlist_ids[0];
    bool is_loved = db::playlist_by_id(loved_tracks_playlist_id)->get().has_track_id(track_id);
    bool is_user_playlist = db::playlist_by_id(playlist_id).value().get().type == db::PlaylistType::User;

    std::vector<std::string> popover_labels;
    std::vector<std::function<void()>> popover_actions;

    popover_labels.emplace_back("Play next");
    popover_actions.emplace_back([track_id, playlist_id, collection_id]() {
      player::enqueue(
        player::playing_t{.collection_id = collection_id, .playlist_id = playlist_id, .track_id = track_id},
        player::get_playing_index().value_or(player::get_playing_queue().size()));
    });

    if (!is_loved) {
      popover_labels.emplace_back("Love track");
      popover_actions.emplace_back([loved_tracks_playlist_id, track_id]() {
        auto& playlist = db::playlist_by_id(loved_tracks_playlist_id)->get();
        playlist.emplace_track_id(track_id);
        panel_tracks->clear();
        panel_tracks->recreate();
      });
    } else {
      popover_labels.emplace_back("Un-love track");
      popover_actions.emplace_back([loved_tracks_playlist_id, track_id, playlist_id, playlist_track_index]() {
        auto& playlist = db::playlist_by_id(loved_tracks_playlist_id)->get();
        if (playlist_id == loved_tracks_playlist_id) {
          playlist.remove_track_by_index(playlist_track_index);
        } else {
          playlist.remove_track_by_id(track_id);
        }
        panel_tracks->clear();
        panel_tracks->recreate();
      });
    }

    popover_labels.emplace_back("Add to playlist...");
    popover_actions.emplace_back([track_id]() {
      show_add_to_playlist_popup(track_id);
    });

    if (is_user_playlist) {
      popover_labels.emplace_back("Remove from playlist");
      popover_actions.emplace_back([playlist_id, playlist_track_index]() {
        auto& playlist = db::playlist_by_id(playlist_id)->get();
        playlist.remove_track_by_index(playlist_track_index);
        panel_tracks->clear();
        panel_tracks->recreate();
      });
    }

    popover_descriptor d{
      .id = "playlist_track_actions",
      .at = widget->get_position(Anchor::BOTTOM),
      .button_labels = popover_labels,
      .button_actions = popover_actions,
    };
    popup_controller->create_popover(d);
  };

  panel_albums->on_playlist_lmb = [&](size_t playlist_id, Widget*) {
    panel_tracks->scroll_to_playlist(playlist_id);
  };

  if (db::collection_count() > 0) {
    panel_tracks->collection_id = 0;
    panel_tracks->clear();
    panel_tracks->recreate();
    panel_albums->recreate(0, album_cover_atlases[0].get());
    panel_top->recreate(0);
  } else {
    panel_tracks->collection_id = std::nullopt;
    panel_tracks->clear();
    panel_tracks->recreate();
    panel_albums->recreate(std::nullopt, nullptr);
    panel_top->recreate(std::nullopt);
  }

  // musicdb::create_some_debug_playlists();
}

void interface::process_input() {
  while (auto cmd = mpris::command_pop()) {
    switch (cmd->type) {
    case mpris::CommandType::PLAY:
      player::resume();
      break;

    case mpris::CommandType::PAUSE:
      player::pause();
      break;

    case mpris::CommandType::PLAY_PAUSE:
      if (player::is_playing()) {
        player::pause();
      } else {
        player::resume();
      }
      break;

    case mpris::CommandType::NEXT:
      player::next_track();
      break;

    case mpris::CommandType::PREVIOUS:
      player::prev_track();
      break;

    case mpris::CommandType::STOP:
      player::stop();
      break;

    case mpris::CommandType::SEEK: {
      i32 target = player::get_current_time_ms() + (i32)(cmd->value);
      player::seek_ms(target);
      break;
    }

    case mpris::CommandType::SET:
      // Absolute seek
      player::seek_ms(static_cast<i32>(cmd->value));
      break;
    }
  }

  static i32 t = 0;
  if (t++ >= 120) {
    t = 0;
    mpris::notify_seeked(player::get_current_time_ms());
  }

  handle_dropped_files();
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
  atlas.add_texture("track_hovered", "./assets/track_hovered.png");
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
  atlas.add_texture("button_add_tab_disabled", "./assets/button_add_tab_disabled.png");
  atlas.add_texture("button_add_tab_hovered", "./assets/button_add_tab_hovered.png");
  atlas.add_texture("button_add_tab_idle", "./assets/button_add_tab_idle.png");
  atlas.add_texture("button_add_tab_pressed", "./assets/button_add_tab_pressed.png");
  atlas.add_texture("button_popover_disabled", "./assets/button_popover_disabled.png");
  atlas.add_texture("button_popover_hovered", "./assets/button_popover_hovered.png");
  atlas.add_texture("button_popover_idle", "./assets/button_popover_idle.png");
  atlas.add_texture("button_popover_pressed", "./assets/button_popover_pressed.png");
  // icons
  atlas.add_texture("love", "./assets/icons/love.png");
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
  atlas.add_texture("settings", "./assets/icons/settings.png");
  atlas.save_to_file("atlas.png");
}

void init_album_cover_atlas(size_t collection_id) {
  ensure(db::collection_count() > collection_id);

  auto& c = db::collection_by_id(collection_id).value().get();
  i32 album_count = c.playlist_ids.size();
  i32 atlas_resolution = std::sqrt(album_count) * 64;
  if (atlas_resolution < 512) {
    atlas_resolution = 512;
  } else if (atlas_resolution < 1024) {
    atlas_resolution = 1024;
  } else if (atlas_resolution < 2048) {
    atlas_resolution = 2048;
  } else {
    atlas_resolution = 2048;
    debug_warn("album_count = ", album_count, ", not supported!");
  }
  i32 count = 0;
  album_cover_atlases[collection_id] = std::make_unique<TextureAtlas>(atlas_resolution, 0, 64);
  album_cover_atlases[collection_id]->add_texture("cover_unknown", "./assets/cover_unknown.png");
  album_cover_atlases[collection_id]->set_fallback_texture("cover_unknown");
  for (size_t playlist_id : db::collection_by_id(collection_id)->get().playlist_ids) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    album_cover_atlases[collection_id]->add_texture(std::to_string(playlist_id), playlist.image, 64, 64);
    if (count++ >= 1023) { break; }
  }
  album_cover_atlases[collection_id]->save_to_file("albums" + std::to_string(collection_id) + ".png");
}

void init_album_cover_atlases() {
  album_cover_atlases.clear();
  auto n = db::collection_count();
  for (size_t i = 0; i < n; i += 1) {
    album_cover_atlases.emplace_back(std::make_unique<TextureAtlas>(512, 0, 64));
    init_album_cover_atlas(i);
  }
}

void create_collections(std::vector<std::string> /* directories */) {
  debug_log("create_collections");
  // for (auto& str : directories) {
  // fs::path path = str;
  // std::string collection_name = path.filename().string();
  // auto collection_id = db::add_collection(utf8_to_utf32(collection_name));
  // db::collection_by_id(collection_id)->add_path(str);
  // }
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

  auto content_utf32 = utf8_to_utf32(content);
  if (dropped_directories.size() > 0) {
    popup_descriptor d{
      .id = "directories_dropped",
      .title = U"Do you want to create the following collections?",
      .content = content_utf32,
      .button_labels = {U"Cancel", U"Add"},
      .button_actions = {nullptr, [dropped_directories]() {
                           create_collections(dropped_directories);
                           init_album_cover_atlases(); // FIXME: do this only for the added collections
                           panel_top->recreate(active_collection_id);
                         }},
    };
    interface::get_popup_controller()->create_popup(d);
  }
}

void delete_collection(size_t /* collection_id */) {
  debug_warn("delete_collection");
  // musicdb::mark_collection_as_tombstone(collection_id);
  // if (active_collection_id.has_value()) {
  //   if (musicdb::get_collection(*active_collection_id)->is_tombstone()) {
  //     panel_top->recreate(std::nullopt);
  //     panel_albums->recreate(std::nullopt, nullptr);
  //     panel_tracks->collection_id = std::nullopt;
  //     panel_tracks->recreate();
  //   } else {
  //     panel_top->recreate(*active_collection_id);
  //     panel_albums->recreate(*active_collection_id, album_cover_atlases[*active_collection_id].get());
  //     panel_tracks->collection_id = *active_collection_id;
  //     panel_tracks->recreate();
  //   }
  // } else {
  //   panel_top->recreate(std::nullopt);
  //   panel_albums->recreate(std::nullopt, nullptr);
  //   panel_tracks->collection_id = std::nullopt;
  //   panel_tracks->recreate();
  // }
}

void show_add_to_playlist_popup(size_t track_id) {
  auto& track = db::track_by_id(track_id)->get();
  std::u32string pretty_track = track.artist + U" - " + track.title;
  popup_descriptor d{
    .id = "add_to_playlist",
    .title = U"Select a playlist to add this track to ",
    .content = pretty_track,
    .button_labels = {U"Cancel"},
    .button_actions{nullptr},
  };
  auto* popup = popup_controller->create_popup(d);
  auto& playlists_view = popup->content.add_child<PanelAlbums>();
  playlists_view.set_width(COVER_WIDTH * 6);
  playlists_view.set_height(std::clamp(ui->get_window_height() - 300, 100, 500));
  popup->set_width(playlists_view.get_width() + 20);
  popup->set_height(playlists_view.get_height() + 60);
  playlists_view.recreate(0, album_cover_atlases[0].get());

  auto* popup_controller_ = popup_controller;
  playlists_view.on_playlist_lmb = [popup_controller_, track_id](size_t playlist_id, Widget*) {
    db::playlist_by_id(playlist_id)->get().add_track(track_id);
    popup_controller_->close_all_popups();
    panel_tracks->clear();
    panel_tracks->recreate();
  };
}
