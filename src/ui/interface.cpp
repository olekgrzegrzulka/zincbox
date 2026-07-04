#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include "common/config.hpp"
#include "common/input.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/mpris.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "interface.hpp"
#include "interface_notifications.hpp"
#include "lib/json.cpp/json.h"
#include "panel_albums.hpp"
#include "panel_controls.hpp"
#include "panel_queue.hpp"
#include "panel_top.hpp"
#include "panel_tracks.hpp"
#include "popup_controller.hpp"
#include "splitter.hpp"
#include "theme.hpp"
#include "tr.hpp"
#include "ui/panel_tracks_track.hpp"
#include "ui/popup_definitions.hpp"
#include "ui/popup_search.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"
#include "lib/nativefiledialog-extended/src/include/nfd.hpp"

static std::optional<size_t> active_collection_id;
static std::vector<float> tracks_scroll_positions;
static std::vector<float> playlists_scroll_positions;
static std::vector<std::u32string> tabs_order;

static std::unique_ptr<UI> ui;
static class ShortcutInterceptor* shortcut_interceptor{};
static PopupController* popup_controller{};
static InterfaceNotifications* notifications{};
static PanelTop* panel_top{};
static PanelTracks* panel_tracks{};
static PanelQueue* panel_queue{};
static PanelAlbums* panel_albums{};
static PanelControls* panel_controls{};
static Splitter* splitter{};

static void init_atlas();
static void add_playlist_art_to_texture_atlas(size_t collection_id);
static void handle_dropped_files();
static void delete_collection(size_t);
static void delete_playlist(size_t);
static void show_collection(size_t);
static void show_queue();
static void show_add_to_playlist_popup(size_t);
static void show_popup_delete_collection(size_t);
static void show_popup_rename_collection(size_t);
static void show_popup_set_sources(size_t);
static void show_popup_delete_playlist(size_t);
static void show_popup_rename_playlist(size_t);

static void show_popover_track_actions(size_t collection_id, size_t playlist_id, size_t track_id,
                                       std::optional<size_t> playlist_track_index, Widget* widget,
                                       bool remove_from_playlist_option = false);
static void show_popover_collection_actions(size_t collection_id, Widget* widget);
static void show_popover_queue_element_actions(size_t queue_index, Widget* widget);
static void show_popover_playlist_actions(size_t playlist_id, Widget* widget, bool play_actions = true);
static void show_popover_playlist_sort_options(size_t playlist_id, Widget* widget);
static void show_popover_create_playlist(Widget*);
static void action_new_playlist();
static void action_new_smart_playlist();
static void action_new_playlist_from_json();

static void recreate_panel_top(bool order = true) {
  if (order) {
    tabs_order.clear();
    for (Tab* tab : panel_top->get_tab_bar()->get_tabs()) {
      tabs_order.push_back(tab->get_label().get_text());
    }
  }
  panel_top->recreate(active_collection_id);
  panel_top->get_tab_bar()->sort_tabs_by_label(tabs_order);
}

class ShortcutInterceptor : public Widget {
  public:
    ShortcutInterceptor(UI& ui_) : Widget(ui_) {}

    void event(Input::InputEventKey& ev) override {
      if (ev.action != Input::KeyAction::RELEASE) { return; }
      bool ctrl = Input::key_pressed(Input::Key::KEY_LEFT_CONTROL) || Input::key_pressed(Input::Key::KEY_RIGHT_CONTROL);
      // bool shift = Input::key_pressed(Input::Key::KEY_LEFT_SHIFT) ||
      //              Input::key_pressed(Input::Key::KEY_RIGHT_SHIFT);
      if (ctrl && ev.key == Input::Key::KEY_F && search_popup_invoked) {
        ev.handled = true;
        search_popup_invoked();
      }
    }

  public:
    std::function<void(void)> search_popup_invoked{};
};

void interface::init() {
  ui = std::make_unique<UI>(1, 1);
  std::string language = config::json().contains("language") && config::json()["language"].isString()
                           ? config::json()["language"].getString()
                           : "en-US";
  theme::load_theme("default", *ui.get(), language);
  db::set_playlists_collection_name(tr::get("collection.playlists_collection_name"));
  db::set_loved_tracks_playlist_name(tr::get("playlist.loved_tracks_playlist_name"));

  init_atlas();

  shortcut_interceptor = &ui->add_widget<ShortcutInterceptor>();
  shortcut_interceptor->search_popup_invoked = []() {
    auto* p = popup_controller->show_popup<PopupSearch>();
    p->on_playlist_lmb = [p](size_t playlist_id, Widget*) {
      auto collection_id = db::collection_of_playlist(playlist_id);
      if (collection_id.has_value()) {
        show_collection(collection_id.value());
        panel_tracks->scroll_to_playlist(playlist_id);
        p->close();
      }
    };

    p->on_track_lmb = [p](size_t collection_id, size_t playlist_id, size_t track_id, Widget*) {
      show_collection(collection_id);
      panel_tracks->scroll_to_track(playlist_id, track_id);
      p->close();
    };
  };
  popup_controller = &ui->add_widget<PopupController>();
  popup_controller->set_is_drawn_on_top(true);
  notifications = &ui->add_widget<InterfaceNotifications>();
  panel_top = &ui->add_widget<PanelTop>();
  panel_tracks = &ui->add_widget<PanelTracks>();
  panel_queue = &ui->add_widget<PanelQueue>();
  panel_queue->set_is_drawn(false);
  splitter = &ui->add_widget<Splitter>();
  panel_albums = &ui->add_widget<PanelAlbums>();
  panel_controls = &ui->add_widget<PanelControls>();

  panel_controls->on_playing_track_lmb = [](Widget*) -> void {
    auto playing = player::get_playing();
    if (playing.has_value()) {
      show_collection(playing->collection_id);
      panel_albums->scroll_to_playlist(playing->playlist_id);
      panel_tracks->scroll_to_track(playing->playlist_id, playing->track_id);
    }
  };

  panel_controls->on_playing_track_rmb = [](Widget* w) -> void {
    auto playing = player::get_playing();
    if (playing.has_value()) {
      show_popover_track_actions(playing->collection_id, playing->playlist_id, playing->track_id, std::nullopt, w,
                                 false);
    }
  };

  panel_top->on_collection_opened = [&](size_t collection_id) -> void { show_collection(collection_id); };

  panel_top->on_queue_view_opened = [&]() { show_queue(); };

  panel_top->on_show_collection_actions_popover = show_popover_collection_actions;

  panel_top->on_add_collection_button_pressed = [&](Widget*) {
    NFD::UniquePathSet out_paths;
    auto result = NFD::PickFolderMultiple(out_paths, (const nfdnchar_t*)nullptr);
    if (result == NFD_OKAY) {
      nfdpathsetsize_t numPaths;
      NFD::PathSet::Count(out_paths, numPaths);
      if (numPaths > 0) {
        nfdpathsetsize_t i;
        std::string collection_name =
          utf32_to_utf8(tr::get("collection.default_name")) + std::to_string(db::collection_count() + 1);
        auto collection_id = db::add_collection(utf8_to_utf32(collection_name));
        for (i = 0; i < numPaths; i += 1) {
          NFD::UniquePathSetPath path;
          NFD::PathSet::GetPath(out_paths, i, path);
          db::add_path_to_collection(collection_id, path.get());
        }
        add_playlist_art_to_texture_atlas(collection_id);
        recreate_panel_top();
        notifications->push(tr::format("notification.added_collection", collection_name));
      }
    }
  };

  panel_tracks->on_track_lmb = [&](size_t collection_id, size_t playlist_id, size_t track_id,
                                   size_t /* playlist_track_index */, WidgetTrack* widget) {
    if (track_id >= db::track_count()) { return; }
    player::playing_t play{
      .collection_id = collection_id,
      .playlist_id = playlist_id,
      .track_id = track_id,
    };
    bool playback_error = !player::play(play, true);
    widget->set_playback_error(playback_error);
  };

  panel_queue->on_queue_element_lmb = [&](size_t queue_index, Widget*) { player::set_playing_index(queue_index); };

  panel_queue->on_queue_element_rmb = show_popover_queue_element_actions;

  panel_tracks->on_track_rmb = [](size_t collection_id, size_t playlist_id, size_t track_id,
                                  size_t playlist_track_index, WidgetTrack* widget) {
    show_popover_track_actions(collection_id, playlist_id, track_id, playlist_track_index, widget, true);
  };

  panel_tracks->on_playlist_sort_button_pressed = [](size_t, size_t playlist_id, Widget* w) {
    show_popover_playlist_sort_options(playlist_id, w);
  };

  panel_tracks->on_playlist_more_options_invoked = [](size_t, size_t playlist_id, Widget* w) {
    show_popover_playlist_actions(playlist_id, w, false);
  };

  panel_albums->on_playlist_lmb = [&](size_t playlist_id, Widget*) -> void {
    panel_tracks->scroll_to_playlist(playlist_id);
  };

  panel_albums->on_playlist_rmb = [&](size_t playlist_id, Widget* w) -> void {
    show_popover_playlist_actions(playlist_id, w);
  };

  panel_albums->on_button_sort_by_pressed = [](Widget* w) {
    std::vector<std::function<void()>> popover_actions;
    std::vector<std::string> popover_labels;
    if (active_collection_id == 0) {
      popover_labels = {utf32_to_utf8(tr::get("sort.playlist.name.asc")),
                        utf32_to_utf8(tr::get("sort.playlist.name.desc"))};
      popover_actions = {
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_AZ; },
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_ZA; },
      };
    } else if (active_collection_id.has_value()) {
      popover_labels = {utf32_to_utf8(tr::get("sort.artist.name.asc")), utf32_to_utf8(tr::get("sort.artist.name.desc")),
                        utf32_to_utf8(tr::get("sort.album.name.asc")), utf32_to_utf8(tr::get("sort.album.name.desc"))};
      popover_actions = {
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::AUTHOR_AZ; },
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::AUTHOR_ZA; },
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_AZ; },
        []() { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_ZA; },
      };
    }
    vec2i at = w->get_position(Anchor::CENTER);
    popover_descriptor d{
      .id = "sort_by",
      .at = at,
      .distance = 4,
      .button_labels = popover_labels,
      .button_actions = popover_actions,
      .show_arrow = false,
    };
    popup_controller->create_popover(d);
  };

  panel_albums->on_add_playlist_button_pressed = [&](Widget* w) -> void { show_popover_create_playlist(w); };

  if (db::collection_count() > 0) {
    active_collection_id = 0;
    panel_tracks->clear();
    panel_tracks->recreate(active_collection_id);
    panel_albums->props.collection_id = 0;
    recreate_panel_top(false);
  } else {
    active_collection_id = std::nullopt;
    panel_tracks->clear();
    panel_tracks->recreate(active_collection_id);
    panel_albums->props.collection_id = std::nullopt;
    recreate_panel_top(false);
  }
}

void interface::input() {
  while (auto cmd = mpris::command_pop()) {
    switch (cmd->type) {
    case mpris::CommandType::PLAY: player::resume(); break;

    case mpris::CommandType::PAUSE: player::pause(); break;

    case mpris::CommandType::PLAY_PAUSE:
      if (player::is_playing()) {
        player::pause();
      } else {
        player::resume();
      }
      break;

    case mpris::CommandType::NEXT: player::next_track(); break;

    case mpris::CommandType::PREVIOUS: player::prev_track(); break;

    case mpris::CommandType::STOP: player::stop(); break;

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
  ui->input();
}

void interface::update(vec2i window_size) {
  i32 height = window_size.y - panel_top->get_height() - panel_controls->get_height();
  panel_top->set_width(window_size.x);

  if (!popup_controller->is_popup_open() && (splitter->is_mouse_hovering() || splitter->get_is_dragged())) {
    Input::set_cursor(Input::Cursor::RESIZE_HORIZONTAL);
  } else {
    Input::set_cursor(Input::Cursor::ARROW);
  }

  if (panel_queue->get_is_drawn()) {
    panel_queue->set_y(panel_top->get_height());
    panel_queue->set_width(window_size.x);
    panel_queue->set_height(height);
  } else {
    i32 width_tracks = window_size.x * splitter->get_ratio() - (i32)(splitter->get_width() / 2);
    width_tracks = std::clamp(width_tracks, 200, window_size.x - 200);
    i32 width_albums = window_size.x - width_tracks - splitter->get_width();
    i32 y = panel_top->get_height();

    panel_tracks->set_pos(0, y);
    panel_tracks->set_width(width_tracks);
    panel_tracks->set_height(height);

    splitter->set_pos(width_tracks, y);
    splitter->set_height(height);

    panel_albums->set_pos(width_tracks + splitter->get_width(), y);
    panel_albums->set_width(width_albums);
    panel_albums->set_height(height);
  }
  panel_controls->set_width(window_size.x);

  ui->update(window_size.x, window_size.y);
}

void interface::draw() { ui->draw(); }

void interface::deinit() { ui = nullptr; }

PopupController* interface::get_popup_controller() { return popup_controller; }

jt::Json interface::to_json() {
  tabs_order.clear();
  for (Tab* tab : panel_top->get_tab_bar()->get_tabs()) {
    tabs_order.push_back(tab->get_label().get_text());
  }

  auto json = jt::Json();
  std::vector<jt::Json> tabs_order_utf8;
  tabs_order_utf8.reserve(tabs_order.size());
  for (const auto& tab : tabs_order) {
    tabs_order_utf8.emplace_back(utf32_to_utf8(tab));
  }
  json["tabs_order"].setArray();
  json["tabs_order"].getArray().clear();
  json["tabs_order"].getArray() = tabs_order_utf8;

  const Tab* selected_tab = panel_top->get_tab_bar()->get_selected_tab();
  if (selected_tab) {
    json["selected_tab"] = utf32_to_utf8(selected_tab->get_label().get_text());
    if (active_collection_id) {
      json["tracks_scroll_offset"] = panel_tracks->get_scroll_px();
      json["playlists_scroll_offset"] = panel_albums->get_scroll_px();
    }
  }

  return json;
}

void interface::from_json(const jt::Json& json) {
  // tabs order
  if (json.contains("tabs_order") && json["tabs_order"].isArray()) {
    auto& json_tabs_order = json["tabs_order"].getArray();
    tabs_order.clear();
    for (const auto& tab : json_tabs_order) {
      if (tab.isString()) { tabs_order.emplace_back(utf8_to_utf32(tab.getString())); }
    }
  }
  panel_top->get_tab_bar()->sort_tabs_by_label(tabs_order);

  // selected tab
  if (json.contains("selected_tab") && json["selected_tab"].isString()) {
    std::u32string selected_tab = utf8_to_utf32(json["selected_tab"].getString());
    const Tab* tab = panel_top->get_tab_bar()->get_tab_by_label(selected_tab);
    if (tab) {
      panel_top->get_tab_bar()->unselect_all_tabs();
      panel_top->get_tab_bar()->select_tab(tab->id);
      if (tab->id < (i32)db::collection_count()) {
        show_collection(tab->id);
      } else {
        show_queue();
      }
    } else {
      show_queue();
    }
  }

  // tracks scroll offset
  if (json.contains("tracks_scroll_offset") && json["tracks_scroll_offset"].isNumber()) {
    panel_tracks->set_scroll_px(json["tracks_scroll_offset"].getNumber());
  }

  // playlists scroll offset
  if (json.contains("playlists_scroll_offset") && json["playlists_scroll_offset"].isNumber()) {
    panel_albums->set_scroll_px(json["playlists_scroll_offset"].getNumber());
  }
}

static void init_atlas() {
  auto& atlas = ui->get_texture_atlas();

  for (size_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    add_playlist_art_to_texture_atlas(collection_id);
  }

  atlas.save_to_file("atlas.png");
}

static void add_playlist_art_to_texture_atlas(size_t collection_id) {
  i32 count = 0;
  if (!db::collection_by_id(collection_id).has_value()) { return; }
  for (size_t playlist_id : db::collection_by_id(collection_id)->get().playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    std::string playlist_id_str = std::to_string(playlist_id);
    if (ui->get_texture_atlas().has_texture(playlist_id_str, 1)) {
      ui->get_texture_atlas().remove_texture(playlist_id_str);
    }
    ui->get_texture_atlas().add_texture(playlist_id_str, playlist.art_64x64, 64, 64);
    if (count++ >= 1023) { break; }
  }
  ui->get_texture_atlas().save_to_file("atlas.png");
}

static void create_collection(std::vector<std::string> directories) {
  if (directories.size() == 0) { return; }
  std::string collection_name = fs::path{directories[0]}.filename().string();
  auto collection_id = db::add_collection(utf8_to_utf32(collection_name));
  for (auto& str : directories) {
    fs::path path = str;
    db::add_path_to_collection(collection_id, path.string());
  }
  add_playlist_art_to_texture_atlas(collection_id);
  recreate_panel_top();
}

static void create_multiple_collections(const std::vector<std::string>& directories) {
  for (auto& str : directories) {
    fs::path path = str;
    std::string collection_name = path.filename().string();
    auto collection_id = db::add_collection(utf8_to_utf32(collection_name));
    db::add_path_to_collection(collection_id, path.string());
    add_playlist_art_to_texture_atlas(collection_id);
  }
  recreate_panel_top();
}

static void handle_dropped_files() {
  std::vector<std::string> dropped_directories{};
  for (auto& path : Input::get_dropped_paths()) {
    if (std::filesystem::is_directory(path)) { dropped_directories.emplace_back(path); }
  }

  if (dropped_directories.empty()) { return; }

  auto* popup = popup_controller->show_popup<PopupImportFolders>(dropped_directories);

  popup->on_add_collections_pressed = [](const std::vector<std::string>& dirs) {
    if (dirs.size() > 1) {
      create_multiple_collections(dirs);
    } else {
      create_collection(dirs);
    }
  };

  popup->on_merge_pressed = [](const std::vector<std::string>& dirs) { create_collection(dirs); };
}

static void delete_collection(size_t collection_id) {
  db::mark_collection_as_tombstone(collection_id);

  if (db::collection_by_id(*active_collection_id)->get().is_tombstone() || !active_collection_id.has_value()) {
    active_collection_id = std::nullopt;
    recreate_panel_top();
    panel_albums->props.collection_id = active_collection_id;
    panel_tracks->recreate(active_collection_id);
  } else {
    recreate_panel_top();
    panel_albums->props.collection_id = active_collection_id;
    panel_tracks->recreate(active_collection_id);
  }
}

static void delete_playlist(size_t playlist_id) {
  db::mark_playlist_as_tombstone(playlist_id);

  panel_albums->recreate();
  panel_tracks->clear();
  panel_tracks->recreate(active_collection_id);
}

static void show_collection(size_t collection_id) {
  if (collection_id == active_collection_id) { return; }
  if (collection_id >= db::collection_count()) { return; }

  if (tracks_scroll_positions.size() <= collection_id) { tracks_scroll_positions.resize(collection_id + 1, 0.0f); }
  if (playlists_scroll_positions.size() <= collection_id) {
    playlists_scroll_positions.resize(collection_id + 1, 0.0f);
  }

  if (active_collection_id.has_value()) {
    tracks_scroll_positions[active_collection_id.value()] = panel_tracks->get_scroll_px();
    playlists_scroll_positions[active_collection_id.value()] = panel_albums->get_scroll_px();
  }

  active_collection_id = collection_id;

  panel_top->select(collection_id);

  panel_tracks->clear();
  panel_tracks->recreate(active_collection_id);
  panel_tracks->set_is_drawn(true);
  panel_tracks->set_scroll_px(tracks_scroll_positions[collection_id]);
  panel_tracks->set_is_updated(true);

  panel_albums->set_is_drawn(true);
  panel_albums->set_is_updated(true);
  panel_albums->set_scroll_px(playlists_scroll_positions[collection_id]);
  panel_albums->props.collection_id = active_collection_id;

  panel_queue->set_is_drawn(false);
  panel_queue->set_is_updated(false);

  splitter->set_is_drawn(true);
  splitter->set_is_updated(true);
}

static void show_queue() {
  active_collection_id = std::nullopt;
  panel_tracks->set_is_drawn(false);
  panel_tracks->set_is_updated(false);
  panel_albums->set_is_drawn(false);
  panel_albums->set_is_updated(false);
  panel_queue->set_is_drawn(true);
  panel_queue->set_is_updated(true);
  panel_queue->on_queue_changed();
  splitter->set_is_drawn(false);
  splitter->set_is_updated(false);
  panel_albums->props.collection_id = std::nullopt;
}

static void show_add_to_playlist_popup(size_t track_id) {
  auto* popup = popup_controller->show_popup<PopupAddToPlaylist>(track_id);

  popup->on_playlist_selected = [track_id](size_t playlist_id) -> void {
    if (db::add_track_id_to_playlist(playlist_id, track_id)) {
      auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.added_track_to_playlist", utf32_to_utf8(track_pretty_name),
                                     utf32_to_utf8(playlist_name)));
    }
    panel_tracks->clear();
    panel_tracks->recreate(active_collection_id);
  };
}

static void show_popup_delete_collection(size_t collection_id) {
  auto collection_name = std::u32string(db::collection_by_id(collection_id)->get().name());
  std::u32string content = tr::format("dialog.confirm.delete_collection.content", utf32_to_utf8(collection_name));

  auto* popup = popup_controller->show_popup<PopupConfirm>(content);
  popup->set_width(300);
  popup->title->set_text(tr::get("dialog.confirm.delete_collection.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.delete"));

  popup->on_ok_pressed = [collection_id]() {
    delete_collection(collection_id);
    notifications->push(tr::format("notification.deleted_collection",
                                   utf32_to_utf8(std::u32string(db::collection_by_id(collection_id)->get().name()))));
  };
}

static void show_popup_rename_collection(size_t collection_id) {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.rename.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.rename"));
  popup->text_input->label.set_text(std::u32string(db::collection_by_id(collection_id)->get().name()));
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup, collection_id]() {
    std::u32string new_name = popup->text_input->label.get_text();
    if (new_name.empty()) { return; }
    db::rename_collection(collection_id, new_name);
    recreate_panel_top();
  };
}

static void show_popup_set_sources(size_t collection_id) {
  auto* popup = popup_controller->show_popup<PopupSetSources>(collection_id);

  popup->on_remove_path_pressed = [collection_id](const std::string& path) -> void {
    db::remove_path_from_collection(collection_id, path);
    if (active_collection_id.has_value() && active_collection_id.value() == collection_id) {
      panel_tracks->recreate(active_collection_id);
      panel_albums->props.collection_id = collection_id;
    }
    popup_controller->close_all_popups();
  };

  popup->on_add_dir_pressed = [collection_id]() -> void {
    NFD::UniquePath out_path;
    if (NFD::PickFolder(out_path, (const nfdnchar_t*)nullptr) == NFD_OKAY) {
      db::add_path_to_collection(collection_id, out_path.get());
      if (active_collection_id.has_value() && active_collection_id.value() == collection_id) {
        panel_tracks->recreate(active_collection_id);
        panel_albums->props.collection_id = collection_id;
      }
    }
  };
}

static void show_popup_delete_playlist(size_t playlist_id) {
  auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
  std::u32string content = tr::format("dialog.confirm.delete_playlist.content", utf32_to_utf8(playlist_name));

  auto* popup = popup_controller->show_popup<PopupConfirm>(content);
  popup->set_width(300);
  popup->title->set_text(tr::get("dialog.confirm.delete_playlist.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.delete"));

  popup->on_ok_pressed = [playlist_id]() {
    delete_playlist(playlist_id);
    notifications->push(
      tr::format("notification.deleted_playlist", utf32_to_utf8(db::playlist_by_id(playlist_id)->get().name)));
  };
}

static void show_popup_rename_playlist(size_t playlist_id) {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.rename.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.rename"));
  popup->text_input->label.set_text(db::playlist_by_id(playlist_id)->get().name);
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup, playlist_id]() {
    std::u32string new_name = popup->text_input->label.get_text();
    if (new_name.empty()) { return; }
    db::rename_playlist(playlist_id, new_name);
    notifications->push(tr::format("notification.renamed_playlist", utf32_to_utf8(new_name)));
    panel_albums->recreate();
  };
}

static void show_popover_track_actions(size_t collection_id, size_t playlist_id, size_t track_id,
                                       std::optional<size_t> playlist_track_index, Widget* widget,
                                       bool remove_from_playlist_option) {
  if (!db::track_by_id(track_id).has_value()) { return; }
  bool is_loved = db::playlist_loved_tracks().has_track_id(track_id);
  bool is_user_playlist = db::playlist_by_id(playlist_id).value().get().type == db::PlaylistType::User;

  std::vector<std::string> popover_labels;
  std::vector<std::function<void()>> popover_actions;

  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.play_next")));
  popover_actions.emplace_back([track_id, playlist_id, collection_id]() {
    player::enqueue(player::playing_t{.collection_id = collection_id, .playlist_id = playlist_id, .track_id = track_id},
                    player::get_playing_index().value_or(player::get_playing_queue().size()));

    notifications->push(
      tr::format("notification.playing_next", utf32_to_utf8(db::track_by_id(track_id)->get().pretty_name())));
  });

  if (!is_loved) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.love")));
    popover_actions.emplace_back([track_id]() {
      auto pretty_name = db::track_by_id(track_id)->get().pretty_name();
      notifications->push(tr::format("notification.loved_track", utf32_to_utf8(pretty_name)));
      db::add_track_id_to_playlist(0, track_id);
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    });
  } else {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.unlove")));
    popover_actions.emplace_back([track_id, playlist_id, playlist_track_index]() {
      auto& loved_tracks_playlist = db::playlist_loved_tracks();
      auto pretty_name = db::track_by_id(track_id)->get().pretty_name();
      if (playlist_id == db::playlist_loved_tracks_id()) {
        if (playlist_track_index.has_value()) {
          loved_tracks_playlist.remove_track_by_index(playlist_track_index.value());
        } else {
          loved_tracks_playlist.remove_track_by_id(track_id);
        }
        notifications->push(tr::format("notification.unloved_track", utf32_to_utf8(pretty_name)));
      } else {
        loved_tracks_playlist.remove_track_by_id(track_id);
        notifications->push(tr::format("notification.unloved_track", utf32_to_utf8(pretty_name)));
      }
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    });
  }

  bool is_album = db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::Album;
  if (!is_album) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.show_in_album")));
    popover_actions.emplace_back([track_id]() -> void {
      auto& track = db::track_by_id(track_id)->get();
      auto album_id = track.originating_album_id;
      if (album_id != db::INVALID_ID) {
        auto collection_id = db::collection_of_playlist(album_id);
        if (collection_id.has_value()) {
          show_collection(collection_id.value());
          panel_tracks->scroll_to_track(album_id, track_id);
        }
      }
    });
  }

  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.add_to_playlist")));
  popover_actions.emplace_back([track_id]() { show_add_to_playlist_popup(track_id); });

  if (remove_from_playlist_option && is_user_playlist) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    popover_labels.emplace_back(
      utf32_to_utf8(tr::format("popover.track.remove_from_playlist", utf32_to_utf8(playlist.name))));
    popover_actions.emplace_back([track_id, playlist_id, playlist_track_index]() {
      auto& playlist = db::playlist_by_id(playlist_id)->get();
      if (playlist_track_index.has_value()) {
        ensure(playlist.track_ids.size() > playlist_track_index.value());
        ensure(track_id == playlist.track_ids[playlist_track_index.value()]);
        if (db::remove_track_index_from_playlist(playlist_id, playlist_track_index.value())) {
          auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
          notifications->push(tr::format("notification.removed_track_from_playlist", utf32_to_utf8(track_pretty_name),
                                         utf32_to_utf8(playlist.name)));
        }
      } else {
        if (db::remove_track_id_from_playlist(playlist_id, track_id)) {
          auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
          notifications->push(tr::format("notification.removed_track_from_playlist", utf32_to_utf8(track_pretty_name),
                                         utf32_to_utf8(playlist.name)));
        }
      }
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    });
  }

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "playlist_track_actions",
    .at = at,
    .distance = 4,
    .button_labels = popover_labels,
    .button_actions = popover_actions,
  };
  popup_controller->create_popover(d);
};

static void show_popover_collection_actions(size_t collection_id, Widget* widget) {
  if (collection_id == 0) { return; }
  vec2i at = widget->get_position(Anchor::CENTER);
  popover_descriptor d{
    .id = "collection_actions",
    .at = at,
    .distance = 10,
    .button_labels = {utf32_to_utf8(tr::get("dialog.action.rename")),
                      utf32_to_utf8(tr::get("dialog.action.set_sources")),
                      utf32_to_utf8(tr::get("dialog.action.rescan")), utf32_to_utf8(tr::get("dialog.action.delete"))},
    .button_actions = {
      [collection_id]() { show_popup_rename_collection(collection_id); },
      [collection_id]() { show_popup_set_sources(collection_id); },
      [collection_id]() {
        db::rescan_collection(collection_id);
        add_playlist_art_to_texture_atlas(collection_id);
        if (active_collection_id.has_value() && active_collection_id.value() == collection_id) {
          panel_tracks->recreate(active_collection_id);
          panel_albums->recreate();
        }
      },
      [collection_id]() { show_popup_delete_collection(collection_id); },
    },
  };
  popup_controller->create_popover(d);
};

static void show_popover_queue_element_actions(size_t queue_index, Widget* widget) {
  auto play = player::get_playing_queue()[queue_index];
  size_t track_id = play.track_id;
  size_t playlist_id = play.playlist_id;
  size_t collection_id = play.collection_id;
  bool is_loved = db::playlist_loved_tracks().has_track_id(track_id);

  std::vector<std::string> popover_labels;
  std::vector<std::function<void()>> popover_actions;

  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.queue.remove")));
  popover_actions.emplace_back([queue_index]() {
    player::remove_from_queue(queue_index);
    panel_queue->on_queue_changed();
  });

  bool is_album = db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::Album;
  popover_labels.emplace_back(
    utf32_to_utf8(is_album ? tr::get("popover.track.show_in_album") : tr::get("popover.track.show_in_playlist")));
  popover_actions.emplace_back([collection_id, playlist_id, track_id]() {
    show_collection(collection_id);
    panel_tracks->scroll_to_track(playlist_id, track_id);
  });
  if (!is_album) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.show_in_album")));
    popover_actions.emplace_back([track_id]() -> void {
      auto& track = db::track_by_id(track_id)->get();
      auto album_id = track.originating_album_id;
      if (album_id != db::INVALID_ID) {
        auto collection_id = db::collection_of_playlist(album_id);
        if (collection_id.has_value()) {
          show_collection(collection_id.value());
          panel_tracks->scroll_to_track(album_id, track_id);
        }
      }
    });
  }

  if (!is_loved) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.love")));
    popover_actions.emplace_back([track_id, queue_index]() {
      if (db::add_track_id_to_playlist(0, track_id)) {
        auto pretty_name = db::track_by_id(track_id)->get().pretty_name();
        notifications->push(tr::format("notification.loved_track", utf32_to_utf8(pretty_name)));
      }
      panel_queue->on_queue_changed_at(queue_index);
    });
  } else {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.unlove")));
    popover_actions.emplace_back([track_id, queue_index]() {
      if (db::remove_track_id_from_playlist(0, track_id)) {
        auto pretty_name = db::track_by_id(track_id)->get().pretty_name();
        notifications->push(tr::format("notification.unloved_track", utf32_to_utf8(pretty_name)));
      }
      panel_queue->on_queue_changed_at(queue_index);
    });
  }

  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.track.add_to_playlist")));
  popover_actions.emplace_back([track_id]() { show_add_to_playlist_popup(track_id); });

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "playlist_track_actions",
    .at = at,
    .distance = 4,
    .button_labels = popover_labels,
    .button_actions = popover_actions,
  };
  popup_controller->create_popover(d);
};

static void show_popover_playlist_actions(size_t playlist_id, Widget* widget, bool play_actions) {
  std::vector<std::string> popover_labels;
  std::vector<std::function<void()>> popover_actions;

  if (play_actions) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.playlist.play")));
    popover_actions.emplace_back([playlist_id]() {
      if (panel_albums->get_collection_id().has_value()) {
        player::play_playlist(*(panel_albums->get_collection_id()), playlist_id, true);
      }
    });

    popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.playlist.play_next")));
    popover_actions.emplace_back([playlist_id]() {
      if (panel_albums->get_collection_id().has_value()) {
        if (player::play_playlist(*(panel_albums->get_collection_id()), playlist_id, false)) {
          notifications->push(
            tr::format("notification.playing_next", utf32_to_utf8(db::playlist_by_id(playlist_id)->get().name)));
        }
      }
    });
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album && playlist_id != 0) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.rename")));
    popover_actions.emplace_back([playlist_id]() { show_popup_rename_playlist(playlist_id); });
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.save_as_json")));
    popover_actions.emplace_back([playlist_id]() {
      auto& playlist = db::playlist_by_id(playlist_id)->get();
      auto json = playlist.to_json();
      NFD::UniquePathN outPath;
      std::string json_filter_label = utf32_to_utf8(tr::get("dialog.filter.json_files"));
      nfdfilteritem_t filterList[1] = {{json_filter_label.c_str(), "json"}};
      auto file_name_utf8 = utf32_to_utf8(playlist.name) + ".json";
      const nfdnchar_t* defaultName = file_name_utf8.c_str();
      auto result = NFD::SaveDialog(outPath, filterList, 1, nullptr, defaultName);
      if (result == NFD_OKAY) {
        nfdnchar_t* path = outPath.get();
        std::string path_str(path);
        std::ofstream out(path_str);
        out << json.toStringPretty();
        out.flush();
        out.close();
      }
    });
  }

  popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.pick_image_file")));
  popover_actions.emplace_back([playlist_id]() {
    NFD::UniquePathN out_path_n;

    std::string image_filter_label = utf32_to_utf8(tr::get("dialog.filter.image_files"));
    nfdfilteritem_t filter_item[1] = {{image_filter_label.c_str(), "png,jpg,jpeg"}};
    auto result = NFD::OpenDialog(out_path_n, filter_item, 1);

    if (result == NFD_OKAY) {
      nfdnchar_t* path = out_path_n.get();
      std::string path_str(path);
      db::set_playlist_image(playlist_id, path_str);
      std::string playlist_id_str = std::to_string(playlist_id);
      ui->get_texture_atlas().remove_texture(playlist_id_str);
      ui->get_texture_atlas().add_texture(playlist_id_str, db::playlist_by_id(playlist_id)->get().art_64x64, 64, 64);
      panel_albums->recreate();
    }
  });

  if (!db::playlist_by_id(playlist_id)->get().art_64x64.empty()) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.reset_image")));
    popover_actions.emplace_back([playlist_id]() {
      db::reset_playlist_image(playlist_id);
      std::string playlist_id_str = std::to_string(playlist_id);
      ui->get_texture_atlas().remove_texture(playlist_id_str);
      ui->get_texture_atlas().add_texture_alias(playlist_id_str, "cover_unknown");
      panel_albums->recreate();
    });
  }

  if (db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::Album) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.show_directory")));
    popover_actions.emplace_back([playlist_id]() {
      auto& playlist = db::playlist_by_id(playlist_id)->get();
      if (playlist.get_tracks_count() > 0) {
        auto& track = db::track_by_id(playlist.get_track_ids()[0])->get();
        fs::path path(track.path);
        std::string dir_str = path.parent_path().string();
#ifdef _WIN32
        std::string command = "explorer \"" + dir_str + "\"";
#elif __APPLE__
        std::string command = "open \"" + dir_str + "\"";
#else
        std::string command = "xdg-open \"" + dir_str + "\"";
#endif
        std::system(command.c_str());
      }
    });
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album && playlist_id != 0) {
    popover_labels.emplace_back(utf32_to_utf8(tr::get("dialog.action.remove")));
    popover_actions.emplace_back([playlist_id]() { show_popup_delete_playlist(playlist_id); });
  }

  vec2i at = widget->get_position(Anchor::CENTER);
  popover_descriptor d{
    .id = "playlist_actions",
    .at = at,
    .distance = 16,
    .button_labels = popover_labels,
    .button_actions = popover_actions,
  };
  popup_controller->create_popover(d);
}

static void show_popover_playlist_sort_options(size_t playlist_id, Widget* widget) {
  std::vector<std::string> popover_labels = {
    utf32_to_utf8(tr::get("sort.artist.name.asc")), utf32_to_utf8(tr::get("sort.artist.name.desc")),
    utf32_to_utf8(tr::get("sort.title.name.asc")), utf32_to_utf8(tr::get("sort.title.name.desc"))};
  std::vector<std::function<void()>> popover_actions = {
    [playlist_id]() -> void {
      db::sort_playlist_by_artist_asc(playlist_id);
      panel_albums->recreate();
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    },
    [playlist_id]() -> void {
      db::sort_playlist_by_artist_desc(playlist_id);
      panel_albums->recreate();
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    },
    [playlist_id]() -> void {
      db::sort_playlist_by_name_asc(playlist_id);
      panel_albums->recreate();
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    },
    [playlist_id]() -> void {
      db::sort_playlist_by_name_desc(playlist_id);
      panel_albums->recreate();
      panel_tracks->clear();
      panel_tracks->recreate(active_collection_id);
    },
  };

  vec2i at = widget->get_position(Anchor::CENTER);
  popover_descriptor d{
    .id = "playlist_sort",
    .at = at,
    .distance = 16,
    .button_labels = popover_labels,
    .button_actions = popover_actions,
  };
  popup_controller->create_popover(d);
}

static void show_popover_create_playlist(Widget* w) {
  std::vector<std::string> popover_labels;
  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.new_playlist.add")));
  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.new_playlist.add_smart")));
  popover_labels.emplace_back(utf32_to_utf8(tr::get("popover.new_playlist.add_from_json")));
  std::vector<std::function<void()>> popover_actions = {action_new_playlist, action_new_smart_playlist,
                                                        action_new_playlist_from_json};
  popover_descriptor d{
    .id = "new_playlist",
    .at = w->get_position(Anchor::CENTER),
    .distance = 4,
    .button_labels = popover_labels,
    .button_actions = popover_actions,
  };
  popup_controller->create_popover(d);
}

static void action_new_playlist() {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.add.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.add"));
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup]() {
    db::add_playlist_to_collection(0, db::Playlist{popup->text_input->label.get_text(), U"", db::PlaylistType::User});
    if (active_collection_id == 0) {
      panel_albums->props.collection_id = 0;
      panel_tracks->recreate(active_collection_id);
    }
  };

  popup->on_cancel_pressed = []() {};
}

static void action_new_smart_playlist() { auto* popup = popup_controller->show_popup<PopupCreateSmartPlaylist>(); }

static void action_new_playlist_from_json() {
  NFD::UniquePath result;
  std::string json_filter_label = utf32_to_utf8(tr::get("dialog.filter.json_files"));
  nfdfilteritem_t filterList[1] = {{json_filter_label.c_str(), "json"}};
  nfdresult_t res = NFD::OpenDialog(result, filterList, 1);
  if (res == NFD_OKAY) {}
}
