#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <glaze/glaze.hpp>
#include <nfd.hpp>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/serialized_state.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "core/scanner.hpp"
#include "core/settings.hpp"
#include "interface.hpp"
#include "interface_notifications.hpp"
#include "panel_albums.hpp"
#include "panel_controls.hpp"
#include "panel_queue.hpp"
#include "panel_top.hpp"
#include "panel_tracks.hpp"
#include "popup_controller.hpp"
#include "splitter.hpp"
#include "theme.hpp"
#include "theme_config.hpp"
#include "ui/popup.hpp"
#include "ui/popup_definitions.hpp"
#include "ui/popup_search.hpp"
#include "ui/popup_settings.hpp"
#include "ui/tab_bar.hpp"
#include "ui/tr.hpp"
#include "ui/widget_playlist_header.hpp"
#include "ui/widget_track.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

static std::optional<size_t> active_collection_id;
static std::vector<float> tracks_scroll_positions;
static std::vector<float> playlists_scroll_positions;
static std::vector<std::string> tabs_order;
static bool search_popup_visible = false;
static std::optional<bool> mini_player = std::nullopt;

static std::optional<PanelTracksSelection> selection_drag;
static std::vector<db::track_info> selection_drag_sorted_top_to_bottom;
static bool selection_drag_is_from_queue = false;
static vec2i selection_drag_start{};
static bool selection_drag_started = false;
static i32 selection_drag_tab_id = -1;
static i32 selection_drag_tab_timer = 0;
static constexpr i32 SELECTION_DRAG_TAB_TIMER = 15;

static std::unique_ptr<Root> root;
static class ShortcutInterceptor* shortcut_interceptor{};
static PopupController* popup_controller{};
static InterfaceNotifications* notifications{};
static ColorRect* bg{};
static PanelTop* panel_top{};
static PanelTracks* panel_tracks{};
static PanelQueue* panel_queue{};
static PanelAlbums* panel_albums{};
static PanelControls* panel_controls{};
static Splitter* splitter{};
static ToolTip* tooltip_drag{};

static void input(vec2i window_size);
static void rebuild();
static void draw();

static void init_atlas();
static void add_playlist_art_to_texture_atlas(db::collection_id_t);
static void handle_dropped_files();
static void handle_drag_and_drop();
static void delete_collection(db::collection_id_t);
static void delete_playlist(size_t);
static void show_collection(db::collection_id_t);
static void show_queue();
static void show_add_to_playlist_popup(db::track_id_t);
static void show_add_to_playlist_popup(std::span<const db::track_id_t>);
static void show_popup_delete_collection(db::collection_id_t);
static void show_popup_rename_collection(db::collection_id_t);
static void show_popup_set_sources(db::collection_id_t);
static void show_popup_delete_playlist(db::playlist_id_t);
static void show_popup_rename_playlist(db::playlist_id_t);
static void show_popover_tracklist_track_actions(db::track_info ti, Widget*, bool remove_from_playlist_option,
                                                 const std::function<void()>& callback_close = nullptr);
static void show_popover_tracklist_tracks_actions(WidgetTrack*, std::span<const db::track_info>,
                                                  const std::function<void()>& callback_close = nullptr);
static void show_popover_collection_actions(db::collection_id_t, Widget*);
static void show_popover_queue_track_actions(db::track_info ti, WidgetTrack* widget);
static void show_popover_queue_tracks_actions(WidgetTrack*, std::span<const db::track_info>,
                                              const std::function<void()>& callback_close = nullptr);
static void show_popover_playlist_actions(db::playlist_id_t, Widget*, bool play_actions = true,
                                          const std::function<void()>& callback_close = nullptr);
static void show_popover_playlist_sort_options(db::playlist_id_t, Widget*);
static void show_popover_create_playlist(Widget*);
static void show_popover_queue_actions(Widget*);
static void
show_popup_new_playlist(const std::function<void(std::optional<db::playlist_id_t>)>& callback_close = nullptr);
static void show_popup_new_smart_playlist();
static void show_dialog_new_playlist_from_json();
static void add_track_to_playlist(db::playlist_id_t, db::track_id_t);
static void add_tracks_to_playlist(db::playlist_id_t, std::span<const db::track_id_t>);
static void remove_track_from_playlist(db::playlist_id_t, db::track_id_t);
static void remove_tracks_from_playlist(db::playlist_id_t, std::span<const db::track_id_t>);
static void love_track(db::track_id_t);
static void love_tracks(std::span<const db::track_id_t>);
static void unlove_track(db::track_id_t);
static void unlove_tracks(std::span<const db::track_id_t>);
static void show_search_popup();
static void show_settings_popup();
static void show_about_popup();
static void quit();

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
    ShortcutInterceptor(Root& ui_) : Widget(ui_) {}

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

void zincbox::ui::init() {
  root = std::make_unique<Root>(1, 1);
  std::string language = zincbox::settings().interface.language;
  std::string theme = zincbox::settings().interface.theme;
  theme::load_theme(theme, *root.get(), language);
  db::set_playlists_collection_name(tr::get("collection.playlists_collection_name"));
  db::set_loved_tracks_playlist_name(tr::get("playlist.loved_tracks_playlist_name"));

  init_atlas();

  shortcut_interceptor = &root->add_widget<ShortcutInterceptor>();
  shortcut_interceptor->search_popup_invoked = show_search_popup;

  bg = &root->add_widget<ColorRect>(theme::config().panel_top.color);
  panel_controls = &root->add_widget<PanelControls>();
  panel_top = &root->add_widget<PanelTop>();
  panel_tracks = &root->add_widget<PanelTracks>();
  panel_queue = &root->add_widget<PanelQueue>();
  splitter = &root->add_widget<Splitter>();
  tooltip_drag = &root->add_widget<ToolTip>("", ToolTipPosition::MANUAL);
  tooltip_drag->set_is_drawn(false);
  tooltip_drag->set_anchor(Anchor::TOP);
  tooltip_drag->set_clamp(false);
  panel_albums = &root->add_widget<PanelAlbums>();
  popup_controller = &root->add_widget<PopupController>();
  notifications = &root->add_widget<InterfaceNotifications>();

  panel_queue->hide();
  panel_tracks->hide();
  panel_albums->hide();
  splitter->set_is_updated(false);
  splitter->set_is_drawn(false);

  if (db::track_count() == 0) { popup_controller->show_popup<PopupWelcome>(); }

  panel_controls->on_playing_track_lmb = [](Widget*) -> void {
    if (mini_player.value_or(false)) { return; }
    auto playing = player::get_playing();
    if (playing.has_value()) {
      bool immediate = active_collection_id != playing->collection_id;
      show_collection(playing->collection_id);
      panel_albums->scroll_to_playlist(playing->playlist_id, immediate);
      panel_tracks->scroll_to_track(playing->playlist_id, playing->track_id, immediate);
    }
  };

  panel_controls->on_playing_track_rmb = [](Widget* w) -> void {
    if (mini_player.value_or(false)) { return; }
    auto playing = player::get_playing();
    if (playing.has_value()) { show_popover_tracklist_track_actions(playing.value(), w, false); }
  };

  panel_controls->on_love_button_pressed = []() -> void {
    if (auto playing = player::get_playing()) {
      if (db::playlist_by_id(0)->get().has_track_id(playing->track_id)) {
        unlove_track(playing->track_id);
      } else {
        love_track(playing->track_id);
      }

      // FIXME: hack to prevent flicker
      if (active_collection_id) {
        panel_tracks->show();
      } else {
        panel_queue->show();
      }
    }
  };

  panel_controls->on_button_expand_player_pressed([]() -> void { set_mini_player(false); });

  panel_top->on_collection_opened = [&](size_t collection_id) -> void { show_collection(collection_id); };
  panel_top->on_queue_view_opened = [&]() { show_queue(); };
  panel_top->on_queue_rmb = show_popover_queue_actions;
  panel_top->on_show_collection_actions_popover = show_popover_collection_actions;
  panel_top->on_minimize_button_pressed = []() -> void { zincbox::window()->minimize(); };
  panel_top->on_maximize_button_pressed = []() -> void {
    if (zincbox::window()->is_maximized()) {
      zincbox::window()->restore();
    } else {
      zincbox::window()->maximize();
    }
  };
  panel_top->on_close_button_pressed = []() -> void { zincbox::stop(); };

  panel_top->on_add_collection_button_pressed = [&](Widget*) {
    NFD::UniquePathSet out_paths;
    auto result = NFD::PickFolderMultiple(out_paths, (const nfdu8char_t*)nullptr);
    if (result == NFD_OKAY) {
      nfdpathsetsize_t numPaths;
      NFD::PathSet::Count(out_paths, numPaths);
      if (numPaths > 0) {
        nfdpathsetsize_t i;
        std::string collection_name = (tr::get("collection.default_name")) + std::to_string(db::collection_count() + 1);
        auto collection_id = db::add_collection(collection_name);
        for (i = 0; i < numPaths; i += 1) {
          NFD::UniquePathSetPathU8 path_utf8;
          NFD::PathSet::GetPath(out_paths, i, path_utf8);
          auto path = utf8_to_path(path_utf8.get());
          db::add_path_to_collection(collection_id, path);
          zincbox::scanner::scan_directory(path, collection_id);
        }
        recreate_panel_top();
        notifications->push(tr::format("notification.added_collection", collection_name));
      }
    }
  };

  panel_top->on_hamburger_button_pressed = [&](Widget* w) -> void {
    decltype(popover_descriptor::buttons) buttons;
    buttons.emplace_back(tr::get("hamburger.search"), show_search_popup, "search");
    buttons.emplace_back(tr::get("hamburger.mini_player"), []() -> void { set_mini_player(true); }, "mini_player");
    buttons.emplace_back(tr::get("hamburger.settings"), show_settings_popup, "settings");
    buttons.emplace_back(tr::get("hamburger.about"), show_about_popup, "about");
    buttons.emplace_back(tr::get("hamburger.quit"), quit, "quit");

    vec2i at = w->get_position(Anchor::CENTER);
    popover_descriptor d{
      .id = "hamburger_menu",
      .title = "",
      .at = at,
      .distance = 10,
      .buttons = buttons,
      .show_arrow = true,
    };
    popup_controller->create_popover(d);
  };

  panel_tracks->on_track_lmb = [&](db::track_info ti, WidgetTrack* widget) {
    if (ti.track_id >= db::track_count()) { return; }
    db::track_info play{
      .collection_id = ti.collection_id,
      .playlist_id = ti.playlist_id,
      .track_id = ti.track_id,
    };
    bool playback_error = !player::play(play, true);
    widget->set_playback_error(playback_error);
  };

  panel_queue->on_track_lmb([&](db::track_info ti, WidgetTrack*) -> void { player::set_playing_index(ti.index); });

  panel_queue->on_track_rmb(show_popover_queue_track_actions);
  panel_queue->on_selection_rmb(
    [](WidgetTrack* widget) -> void { show_popover_queue_tracks_actions(widget, panel_queue->selection().get()); });
  panel_tracks->on_track_rmb = [](db::track_info ti, WidgetTrack* widget) {
    show_popover_tracklist_track_actions(ti, widget, true);
  };

  panel_tracks->on_selection_rmb = [](WidgetTrack* widget) -> void {
    show_popover_tracklist_tracks_actions(widget, panel_tracks->selection().get());
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
    decltype(popover_descriptor::buttons) buttons;
    if (active_collection_id == 0) {
      buttons.emplace_back(
        tr::get("sort.playlist.name.asc"), []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_AZ; },
        "name_asc");
      buttons.emplace_back(
        tr::get("sort.playlist.name.desc"),
        []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_ZA; }, "name_desc");
    } else if (active_collection_id.has_value()) {
      buttons.emplace_back(
        tr::get("sort.artist.name.asc"), []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::AUTHOR_AZ; },
        "artist_asc");
      buttons.emplace_back(
        tr::get("sort.artist.name.desc"),
        []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::AUTHOR_ZA; }, "artist_desc");
      buttons.emplace_back(
        tr::get("sort.album.name.asc"), []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_AZ; },
        "name_asc");
      buttons.emplace_back(
        tr::get("sort.album.name.desc"), []() -> void { panel_albums->props.sort_by = PanelAlbums::SortBy::NAME_ZA; },
        "name_desc");
    }
    vec2i at = w->get_position(Anchor::CENTER);
    popover_descriptor d{
      .id = "sort_by",
      .title = "",
      .at = at,
      .distance = 10,
      .buttons = buttons,
      .show_arrow = true,
    };
    popup_controller->create_popover(d);
  };

  panel_albums->on_add_playlist_button_pressed = [&](Widget* w) -> void { show_popover_create_playlist(w); };

  recreate_panel_top(false);
}

static void input(vec2i window_size) {
  handle_dropped_files();
  handle_drag_and_drop();
  root->input(window_size.x, window_size.y);
}

static void rebuild() { root->rebuild(); }

void zincbox::ui::update(vec2i window_size) {

  auto scan_progress = zincbox::scanner::get_progress();
  if (scan_progress) {
    out::info("Scanned {} files and {} directories", scan_progress->files_scanned, scan_progress->directories_scanned);
  }

  auto scan_summary = zincbox::scanner::import();
  if (scan_summary) {

    add_playlist_art_to_texture_atlas(scan_summary->collection_id);
    if (active_collection_id.has_value() && active_collection_id.value() == scan_summary->collection_id) {
      panel_tracks->recreate(active_collection_id);
      panel_albums->props.collection_id = scan_summary->collection_id;
      panel_albums->recreate();
    }

    out::info(" Import for collection '{}' (id = {}) finished",
              db::collection_by_id(scan_summary->collection_id)->get().name(), scan_summary->collection_id);
    out::info("added tracks:      {}", scan_summary->added_tracks.size());
    out::info("changed tracks:    {}", scan_summary->modified_tracks.size());
    out::info("unchanged tracks:  {}", scan_summary->skipped_tracks.size());
    out::info("tracks not found:  {}", scan_summary->not_found_tracks.size());
    if (scan_summary->errors.size() > 0) { out::error("errors:            {}", scan_summary->errors.size()); }
  }

  input(window_size);
  rebuild();

  auto& window_decor = theme::config().custom_window_decoration;
  bool mini_player_disabled = !mini_player.value_or(false);
  i32 border = 0;
  if (window_decor.enabled && mini_player_disabled) { border = window_decor.border_size; }

  i32 content_x = border;
  i32 content_width = window_size.x - (border * 2);
  i32 content_y = border + panel_top->get_height();
  i32 content_height = window_size.y - (border * 2) - panel_top->get_height() - panel_controls->get_height();

  panel_top->set_pos(content_x, border);
  panel_top->set_width(content_width);

  if (!popup_controller->is_popup_open() && (splitter->is_mouse_hovering() || splitter->get_is_dragged()) &&
      splitter->get_is_drawn()) {
    Input::set_cursor(Input::Cursor::RESIZE_HORIZONTAL);
  } else {
    Input::reset_cursor();
  }

  bg->set_size(window_size);

  panel_queue->set_pos(content_x, content_y);
  panel_queue->set_width(content_width);
  panel_queue->set_height(content_height);

  i32 width_tracks = (content_width * splitter->get_ratio()) - (i32)(splitter->get_width() / 2);
  width_tracks = std::clamp(width_tracks, 200, std::max(content_width - 200, 200));
  i32 width_albums = content_width - width_tracks - splitter->get_width();

  panel_tracks->set_pos(content_x, content_y);
  panel_tracks->set_width(width_tracks);
  panel_tracks->set_height(content_height);

  splitter->set_pos(content_x + width_tracks, content_y);
  splitter->set_height(content_height);

  panel_albums->set_pos(content_x + width_tracks + splitter->get_width(), content_y);
  panel_albums->set_width(width_albums);
  panel_albums->set_height(content_height);

  panel_controls->set_pos(content_x, -border);
  panel_controls->set_width(content_width);

  root->update();
  draw();
}

static void draw() { root->draw(); }

void zincbox::ui::deinit() { root = nullptr; }

zincbox::ui::DecorationHover zincbox::ui::get_decoration_hover(i32 mouse_x, i32 mouse_y) {
  vec2i mouse_pos{mouse_x, mouse_y};
  if (!theme::config().custom_window_decoration.enabled) { return DecorationHover::INSIDE; }
  i32 border_size = theme::config().custom_window_decoration.border_size;
  if (border_size < 6) { border_size = 6; }
  i32 w = root->get_window_width();
  i32 h = root->get_window_height();

  bool mini_player_disabled = !mini_player.value_or(false);

  if (mini_player_disabled) {
    if (mouse_pos.x < border_size && mouse_pos.y < border_size) { return DecorationHover::TOP_LEFT; }
    if (mouse_pos.x >= w - border_size && mouse_pos.y < border_size) { return DecorationHover::TOP_RIGHT; }
    if (mouse_pos.x < border_size && mouse_pos.y >= h - border_size) { return DecorationHover::BOTTOM_LEFT; }
    if (mouse_pos.x >= w - border_size && mouse_pos.y >= h - border_size) { return DecorationHover::BOTTOM_RIGHT; }
  }

  if (mouse_pos.y < border_size && mini_player_disabled) { return DecorationHover::TOP; }
  if (mouse_pos.y >= h - border_size && mini_player_disabled) { return DecorationHover::BOTTOM; }
  if (mouse_pos.x < border_size) { return DecorationHover::LEFT; }
  if (mouse_pos.x >= w - border_size) { return DecorationHover::RIGHT; }
  if (mini_player_disabled) {
    if (panel_top->is_mouse_hovering() && panel_top->can_drag_window()) { return DecorationHover::TITLEBAR; }
  } else {
    if (panel_controls->can_drag_window()) { return DecorationHover::TITLEBAR; }
  }

  return DecorationHover::INSIDE;
}

static void init_atlas() {
  auto& atlas = root->get_texture_atlas();

  for (size_t collection_id = 0; collection_id < db::collection_count(); collection_id += 1) {
    add_playlist_art_to_texture_atlas(collection_id);
  }

#ifndef NDEBUG
  atlas.save_to_file("atlas.png");
#endif
}

static void add_playlist_art_to_texture_atlas(db::collection_id_t collection_id) {
  i32 count = 0;
  if (!db::collection_by_id(collection_id).has_value()) { return; }
  for (size_t playlist_id : db::collection_by_id(collection_id)->get().playlist_ids()) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    std::string playlist_id_str = std::to_string(playlist_id);
    if (root->get_texture_atlas().has_texture(playlist_id_str, 1)) {
      root->get_texture_atlas().remove_texture(playlist_id_str);
    }
    root->get_texture_atlas().add_texture(playlist_id_str, playlist.art_64x64, 64, 64);
    if (count++ >= 1023) { break; }
  }
#ifndef NDEBUG
  root->get_texture_atlas().save_to_file("atlas.png");
#endif
}

static void create_collection(std::vector<std::string> directories) {
  if (directories.size() == 0) { return; }
  std::string collection_name = path_to_utf8(fs::path{directories[0]}.filename());
  auto collection_id = db::add_collection(collection_name);
  for (auto& str : directories) {
    fs::path path = str;
    db::add_path_to_collection(collection_id, path);
    zincbox::scanner::scan_directory(path, collection_id);
  }
  recreate_panel_top();
}

static void create_multiple_collections(const std::vector<std::string>& directories) {
  for (auto& str : directories) {
    fs::path path = utf8_to_path(str);
    std::string collection_name = path_to_utf8(path.filename());
    auto collection_id = db::add_collection(collection_name);
    db::add_path_to_collection(collection_id, path);
    zincbox::scanner::scan_directory(path, collection_id);
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

static void handle_drag_and_drop() {
  bool lmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool rmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_RIGHT);
  bool lmb_pressed = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb_just_released = Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb = lmb_just_pressed || lmb_pressed || lmb_just_released;

  WidgetAlbumCover* hovered_playlist_cover = nullptr;
  WidgetTrack* hovered_track = nullptr;
  WidgetPlaylistHeader* hovered_playlist_header = nullptr;
  bool hovered_queue_panel = false;
  Tab* hovered_tab = nullptr;
  if (lmb) {
    for (Widget* hovered_widget : root->get_hovered_widgets()) {
      if (hovered_playlist_cover = dynamic_cast<WidgetAlbumCover*>(hovered_widget); hovered_playlist_cover) { break; }
      if (hovered_track = dynamic_cast<WidgetTrack*>(hovered_widget); hovered_track) { break; }
      if (hovered_playlist_header = dynamic_cast<WidgetPlaylistHeader*>(hovered_widget); hovered_playlist_header) {
        break;
      }
      if (hovered_tab = dynamic_cast<Tab*>(hovered_widget); hovered_tab) {
        if (selection_drag_tab_id != hovered_tab->id) {
          selection_drag_tab_id = hovered_tab->id;
          selection_drag_tab_timer = 0;
        }
        break;
      }
    }
  }

  if (!popup_controller->is_popup_open() && lmb_just_pressed) {
    if (panel_tracks->get_is_drawn()) {
      if (!panel_tracks->selection().empty()) {
        selection_drag_start = Input::get_mouse_pos();
        selection_drag = panel_tracks->selection();
        selection_drag_is_from_queue = false;
      } else if (hovered_track) {
        selection_drag_start = Input::get_mouse_pos();
        selection_drag = PanelTracksSelection{};
        selection_drag->insert(hovered_track->track_info());
        selection_drag_is_from_queue = false;
      }
    } else if (panel_queue->get_is_drawn()) {
      if (!panel_queue->selection().empty()) {
        selection_drag_start = Input::get_mouse_pos();
        selection_drag = panel_queue->selection();
        selection_drag_is_from_queue = true;
      } else if (hovered_track) {
        selection_drag_start = Input::get_mouse_pos();
        selection_drag = PanelTracksSelection{};
        selection_drag->insert(hovered_track->track_info());
        selection_drag_is_from_queue = true;
      }
    }
  }

  auto handle_drag_playlist = [&](db::playlist_id_t playlist_id, bool drag_ended) -> bool {
    auto playlist = db::playlist_by_id(playlist_id);
    if (!playlist.has_value()) { return false; }
    if (!selection_drag.has_value()) { return false; }

    if (drag_ended) {
      if (playlist->get().type != db::PlaylistType::User) { return false; }
      std::vector<db::track_id_t> track_ids;
      track_ids.reserve(selection_drag.value().size());

      for (auto& ti : selection_drag.value().get()) {
        track_ids.emplace_back(ti.track_id);
      }
      add_tracks_to_playlist(playlist_id, track_ids);
      return true;
    } else {
      if (playlist->get().type != db::PlaylistType::User) {
        tooltip_drag->set_text(tr::get("tooltip.drag.not_allowed"));
        return true;
      }
      if (selection_drag->size() > 1) {
        tooltip_drag->set_text(
          tr::format("tooltip.drag.add_to_playlist_plural", selection_drag->size(), playlist->get().name));
      } else {
        tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist", playlist->get().name));
      }
      return true;
    }
  };

  auto handle_drag_create_playlist = [&](bool drag_ended) -> bool {
    if (!selection_drag.has_value()) { return false; }

    if (drag_ended) {
      std::vector<db::track_id_t> track_ids;
      track_ids.reserve(selection_drag.value().size());

      for (auto& ti : selection_drag.value().get()) {
        track_ids.emplace_back(ti.track_id);
      }

      show_popup_new_playlist([track_ids](std::optional<db::playlist_id_t> playlist_id) -> void {
        if (playlist_id.has_value()) { add_tracks_to_playlist(playlist_id.value(), track_ids); }
      });

      return true;
    } else {
      tooltip_drag->set_text(tr::get("tooltip.drag.create_playlist"));
      return true;
    }
  };

  auto handle_drag_playlist_cover = [&](WidgetAlbumCover* hovered_playlist_cover_, bool drag_ended) -> bool {
    if (!hovered_playlist_cover_) { return false; }
    if (hovered_playlist_cover_->is_add_button()) {
      return handle_drag_create_playlist(drag_ended);
    } else {
      if (!hovered_playlist_cover_->playlist_id.has_value()) { return false; }
      return handle_drag_playlist(hovered_playlist_cover_->playlist_id.value(), drag_ended);
    }
  };

  auto handle_drag_playlist_header = [&](WidgetPlaylistHeader* hovered_playlist_header_, bool drag_ended) -> bool {
    if (!hovered_playlist_header_) { return false; }
    return handle_drag_playlist(hovered_playlist_header_->playlist_id, drag_ended);
  };

  panel_tracks->set_insert_cursor_track_info(std::nullopt);
  panel_queue->set_insert_cursor_track_info(std::nullopt);

  auto handle_drag_tab = [&](Tab* hovered_tab_, bool /* drag_ended */) -> bool {
    if (!hovered_tab_) { return false; }
    if (selection_drag_tab_timer < SELECTION_DRAG_TAB_TIMER) {
      selection_drag_tab_timer += 1;
    } else {
      panel_top->select(hovered_tab_->id);
    }
    return false;
  };

  auto handle_drag_track_queue = [&](WidgetTrack* hovered_track_, bool drag_ended) -> bool {
    if (!selection_drag.has_value()) { return false; }
    if (!hovered_track_) { return false; }
    auto playlist = db::playlist_by_id(hovered_track_->playlist_id());
    if (!playlist.has_value()) { return false; }
    bool above = hovered_track_->get_position(Anchor::CENTER).y > Input::get_mouse_y();
    if (!drag_ended) {
      auto insert_cursor_pos = above ? PanelTracks::InsertCursorPos::ABOVE : PanelTracks::InsertCursorPos::BELOW;
      panel_queue->set_insert_cursor_track_info(hovered_track_->track_info());
      panel_queue->set_insert_cursor_pos(insert_cursor_pos);
      bool move = selection_drag_is_from_queue;
      if (move) {
        if (selection_drag->size() > 1) {
          tooltip_drag->set_text(tr::format("tooltip.drag.reorder_plural", selection_drag->size()));
        } else {
          tooltip_drag->set_text(tr::get("tooltip.drag.reorder"));
        }
      } else {
        if (selection_drag->size() > 1) {
          tooltip_drag->set_text(tr::format("tooltip.drag.add_to_queue_plural", selection_drag->size()));
        } else {
          tooltip_drag->set_text(tr::get("tooltip.drag.add_to_queue"));
        }
      }
      return true;
    } else {
      bool move = selection_drag_is_from_queue;
      size_t target_index = hovered_track_->track_info().index;
      if (!above) { target_index += 1; }
      if (move) {
        std::vector<size_t> indices;
        indices.reserve(selection_drag_sorted_top_to_bottom.size());
        for (const auto& item : selection_drag_sorted_top_to_bottom) {
          indices.emplace_back(item.index);
        }
        player::move_queue_tracks(indices, target_index);
      } else {
        player::add_to_queue(selection_drag_sorted_top_to_bottom, target_index);
      }
      panel_queue->on_queue_changed();
      return true;
    }
  };

  auto handle_drag_track_tracklist = [&](WidgetTrack* hovered_track_, bool drag_ended) -> bool {
    if (!selection_drag.has_value()) { return false; }
    if (!hovered_track_) { return false; }
    auto target_playlist_id = hovered_track_->playlist_id();
    auto playlist = db::playlist_by_id(target_playlist_id);
    if (!playlist.has_value()) { return false; }
    size_t i = hovered_track_->track_info().index;
    bool above = hovered_track_->get_position(Anchor::CENTER).y > Input::get_mouse_y();
    bool move = !selection_drag_is_from_queue && selection_drag->get_common_playlist_id() == target_playlist_id;
    if (drag_ended) {
      if (playlist->get().type != db::PlaylistType::User) { return false; }
      if (!above) { i += 1; }
      std::vector<db::track_id_t> track_ids;
      track_ids.reserve(selection_drag_sorted_top_to_bottom.size());
      std::vector<size_t> indices_to_remove;
      if (move) { indices_to_remove.reserve(selection_drag_sorted_top_to_bottom.size()); }
      for (const auto& item : selection_drag_sorted_top_to_bottom) {
        track_ids.emplace_back(item.track_id);
        if (move) {
          indices_to_remove.emplace_back(item.index);
          if (item.index < i) { i -= 1; }
        }
      }
      if (move) {
        db::remove_track_indices_from_playlist(target_playlist_id, indices_to_remove);
        db::add_track_ids_to_playlist(target_playlist_id, i, track_ids);
      } else {
        db::add_track_ids_to_playlist(target_playlist_id, i, track_ids);
      }
      if (active_collection_id.has_value()) {
        panel_tracks->clear();
        panel_tracks->recreate(active_collection_id);
      }
      return true;
    } else {
      auto insert_cursor_pos = above ? PanelTracks::InsertCursorPos::ABOVE : PanelTracks::InsertCursorPos::BELOW;
      panel_tracks->set_insert_cursor_track_info(hovered_track_->track_info());
      panel_tracks->set_insert_cursor_pos(insert_cursor_pos);
      if (playlist->get().type != db::PlaylistType::User) {
        tooltip_drag->set_text(tr::get("tooltip.drag.not_allowed"));
      } else if (move) {
        if (selection_drag->size() > 1) {
          tooltip_drag->set_text(tr::format("tooltip.drag.reorder_plural", selection_drag->size()));
        } else {
          tooltip_drag->set_text(tr::get("tooltip.drag.reorder"));
        }
      } else {
        if (playlist.has_value()) {
          auto playlist_name = playlist->get().name;
          if (selection_drag->size() > 1) {
            tooltip_drag->set_text(
              tr::format("tooltip.drag.add_to_playlist_plural", selection_drag->size(), playlist_name));
          } else {
            tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist", playlist_name));
          }
        }
      }
      return true;
    }
  };

  auto handle_drag_track = [&](WidgetTrack* hovered_track_, bool drag_ended) -> bool {
    if (!hovered_track_) { return false; }
    if (panel_queue->get_is_drawn()) {
      return handle_drag_track_queue(hovered_track_, drag_ended);
    } else if (panel_tracks->get_is_drawn()) {
      return handle_drag_track_tracklist(hovered_track_, drag_ended);
    }
    return false;
  };

  auto handle_drag_track_queue_panel = [&](bool drag_ended) -> bool {
    if (!selection_drag.has_value()) { return false; }
    bool move = selection_drag_is_from_queue;
    if (!drag_ended) {
      if (!player::get_playing_queue().empty()) {
        auto info = player::get_playing_queue().back();
        info.index = player::get_playing_queue().size() - 1;
        panel_queue->set_insert_cursor_track_info(info);
        panel_queue->set_insert_cursor_pos(PanelTracks::InsertCursorPos::BELOW);
      }
      if (move) {
        if (selection_drag->size() > 1) {
          tooltip_drag->set_text(tr::format("tooltip.drag.reorder_plural", selection_drag->size()));
        } else {
          tooltip_drag->set_text(tr::get("tooltip.drag.reorder"));
        }
      } else {
        if (selection_drag->size() > 1) {
          tooltip_drag->set_text(tr::format("tooltip.drag.add_to_queue_plural", selection_drag->size()));
        } else {
          tooltip_drag->set_text(tr::get("tooltip.drag.add_to_queue"));
        }
      }
      return true;
    } else {
      size_t target_index = player::get_playing_queue().size();
      if (move) {
        std::vector<size_t> indices;
        indices.reserve(selection_drag_sorted_top_to_bottom.size());
        for (const auto& item : selection_drag_sorted_top_to_bottom) {
          indices.emplace_back(item.index);
        }
        player::move_queue_tracks(indices, target_index);
      } else {
        player::add_to_queue(selection_drag_sorted_top_to_bottom, target_index);
      }
      panel_queue->on_queue_changed();
      return true;
    }
  };

  if (lmb_just_pressed && hovered_track && selection_drag && selection_drag->has(hovered_track->track_info())) {
    selection_drag_started = true;
    selection_drag_sorted_top_to_bottom.clear();
    if (panel_queue->get_is_drawn()) {
      for (auto& item : panel_queue->get_items()) {
        if (selection_drag->has(item.track_info)) { selection_drag_sorted_top_to_bottom.emplace_back(item.track_info); }
      }
    } else if (panel_tracks->get_is_drawn()) {
      for (auto& item : panel_tracks->get_items()) {
        if (selection_drag->has(item.track_info)) { selection_drag_sorted_top_to_bottom.emplace_back(item.track_info); }
      }
    }
  }

  vec2i diff = selection_drag_start - Input::get_mouse_pos();
  bool valid_selection = selection_drag && !selection_drag->empty() && selection_drag_started &&
                         (std::abs(diff.x) > 4 || std::abs(diff.y) > 4);

  if (valid_selection) {
    tooltip_drag->set_is_drawn(true);
    tooltip_drag->set_pos(Input::get_mouse_pos() + vec2i{0, 10});
    panel_tracks->set_is_dragged(true);
    panel_queue->set_is_dragged(true);

    if ((hovered_playlist_cover && handle_drag_playlist_cover(hovered_playlist_cover, false)) ||
        (hovered_playlist_header && handle_drag_playlist_header(hovered_playlist_header, false)) ||
        (hovered_track && handle_drag_track(hovered_track, false)) ||
        (hovered_queue_panel && handle_drag_track_queue_panel(false)) ||
        (hovered_tab && handle_drag_tab(hovered_tab, false))) {
    } else {
      if (selection_drag->size() > 1) {
        tooltip_drag->set_text(tr::format("tooltip.drag.tracks", selection_drag->size()));
      } else {
        tooltip_drag->set_text(tr::get("tooltip.drag.track"));
      }
    }
  } else {
    tooltip_drag->set_is_drawn(false);
  }
  if (lmb_just_released && valid_selection) {
    if (handle_drag_playlist_cover(hovered_playlist_cover, true) ||
        handle_drag_playlist_header(hovered_playlist_header, true) || handle_drag_track(hovered_track, true) ||
        handle_drag_track_queue_panel(true) || handle_drag_tab(hovered_tab, true)) {
      panel_tracks->clear_selection();
      panel_queue->clear_selection();
      selection_drag_sorted_top_to_bottom.clear();
      panel_tracks->set_is_dragged(false);
      panel_queue->set_is_dragged(false);
    }
  }

  if (lmb_just_released || rmb_just_pressed || Input::key_just_pressed(Input::Key::KEY_ESCAPE) ||
      popup_controller->is_popup_open()) {
    selection_drag = std::nullopt;
    selection_drag_started = false;
    selection_drag_tab_id = -1;
    selection_drag_tab_timer = 0;
    panel_tracks->set_is_dragged(false);
    panel_queue->set_is_dragged(false);
  }
}

static void delete_collection(db::collection_id_t collection_id) {
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

static void show_collection(db::collection_id_t collection_id) {
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

  panel_tracks->recreate(active_collection_id);
  panel_tracks->set_scroll_px(tracks_scroll_positions[collection_id], true);
  panel_tracks->show();

  panel_albums->props.collection_id = active_collection_id;
  panel_albums->recreate();
  panel_albums->set_scroll_px(playlists_scroll_positions[collection_id], true);
  panel_albums->show();

  panel_queue->hide();

  splitter->set_is_drawn(true);
  splitter->set_is_updated(true);
}

static void show_queue() {
  active_collection_id = std::nullopt;
  panel_tracks->hide();
  panel_albums->hide();
  panel_queue->show();

  splitter->set_is_drawn(false);
  splitter->set_is_updated(false);

  panel_top->select(panel_top->get_queue_tab()->id);
}

static void show_add_to_playlist_popup(db::track_id_t track_id) {
  auto* popup = popup_controller->show_popup<PopupAddToPlaylist>(track_id);

  popup->on_playlist_selected = [track_id](size_t playlist_id) -> void {
    if (playlist_id == 0) {
      love_track(track_id);
    } else if (db::add_track_id_to_playlist(playlist_id, track_id)) {
      auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.added_track_to_playlist", track_pretty_name, playlist_name));
      panel_tracks->recreate(active_collection_id);
    }
  };
}

static void show_add_to_playlist_popup(std::span<const db::track_id_t> track_ids_) {
  auto* popup = popup_controller->show_popup<PopupAddToPlaylist>(std::nullopt);

  popup->on_playlist_selected = [track_ids =
                                   std::vector(track_ids_.begin(), track_ids_.end())](size_t playlist_id) -> void {
    if (playlist_id == 0) {
      love_tracks(track_ids);
      return;
    }
    i32 tracks_added_count = 0;
    auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
    for (db::track_id_t track_id : track_ids) {
      if (db::add_track_id_to_playlist(playlist_id, track_id)) { tracks_added_count += 1; }
    }
    if (tracks_added_count != 0) {
      notifications->push(tr::format("notification.added_tracks_to_playlist", tracks_added_count, playlist_name));
      panel_tracks->recreate(active_collection_id);
    }
  };
}

static void show_popup_delete_collection(db::collection_id_t collection_id) {
  auto collection_name = std::string(db::collection_by_id(collection_id)->get().name());
  std::string content = tr::format("dialog.confirm.delete_collection.content", collection_name);

  auto* popup = popup_controller->show_popup<PopupConfirm>(content);
  popup->set_width(300);
  popup->title->set_text(tr::get("dialog.confirm.delete_collection.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.delete"));

  popup->on_ok_pressed = [collection_id]() {
    delete_collection(collection_id);
    notifications->push(
      tr::format("notification.deleted_collection", std::string(db::collection_by_id(collection_id)->get().name())));
  };
}

static void show_popup_rename_collection(db::collection_id_t collection_id) {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.rename.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.rename"));
  popup->text_input->label.set_text(std::string(db::collection_by_id(collection_id)->get().name()));
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup, collection_id]() {
    std::string new_name = popup->text_input->label.get_text();
    if (new_name.empty()) { return; }
    db::rename_collection(collection_id, new_name);
    recreate_panel_top();
  };
}

static void show_popup_set_sources(db::collection_id_t collection_id) {
  auto* popup = popup_controller->show_popup<PopupSetSources>(collection_id);

  popup->on_remove_path_pressed = [collection_id](const std::string& path) -> void {
    db::remove_path_from_collection(collection_id, utf8_to_path(path));
    for (auto& path_ : db::collection_by_id(collection_id)->get().paths()) {
      zincbox::scanner::scan_directory(path_, collection_id);
    }

    if (active_collection_id.has_value() && active_collection_id.value() == collection_id) {
      panel_tracks->recreate(active_collection_id);
      panel_albums->props.collection_id = collection_id;
    }
    popup_controller->close_all_popups();
  };

  popup->on_add_dir_pressed = [collection_id]() -> void {
    NFD::UniquePathU8 out_path;
    if (NFD::PickFolder(out_path, (const nfdu8char_t*)nullptr) == NFD_OKAY) {
      fs::path dir = utf8_to_path(out_path.get());
      zincbox::scanner::scan_directory(dir, collection_id);
    }
  };
}

static void show_popup_delete_playlist(db::playlist_id_t playlist_id) {
  auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
  std::string content = tr::format("dialog.confirm.delete_playlist.content", playlist_name);

  auto* popup = popup_controller->show_popup<PopupConfirm>(content);
  popup->set_width(300);
  popup->title->set_text(tr::get("dialog.confirm.delete_playlist.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.delete"));

  popup->on_ok_pressed = [playlist_id]() {
    delete_playlist(playlist_id);
    notifications->push(tr::format("notification.deleted_playlist", db::playlist_by_id(playlist_id)->get().name));
  };
}

static void show_popup_rename_playlist(db::playlist_id_t playlist_id) {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.rename.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.rename"));
  popup->text_input->label.set_text(db::playlist_by_id(playlist_id)->get().name);
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup, playlist_id]() {
    std::string new_name = popup->text_input->label.get_text();
    if (new_name.empty()) { return; }
    db::rename_playlist(playlist_id, new_name);
    notifications->push(tr::format("notification.renamed_playlist", new_name));
    panel_albums->recreate();
  };
}

static void show_popover_tracklist_track_actions(db::track_info ti, Widget* widget, bool remove_from_playlist_option,
                                                 const std::function<void()>& callback_close) {
  if (!db::track_by_id(ti.track_id).has_value()) { return; }
  bool is_loved = db::playlist_loved_tracks().has_track_id(ti.track_id);
  bool is_user_playlist = db::playlist_by_id(ti.playlist_id).value().get().type == db::PlaylistType::User;

  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("popover.track.play"),
    [ti, callback_close]() -> void {
      player::play(ti, true);
      if (callback_close) { callback_close(); }
    },
    "play_track");

  buttons.emplace_back(
    tr::get("popover.track.play_next"),
    [ti, callback_close]() -> void {
      player::enqueue(ti, player::get_playing_index().value_or(player::get_playing_queue().size()));

      notifications->push(
        tr::format("notification.appended_to_queue", db::track_by_id(ti.track_id)->get().pretty_name()));
      if (callback_close) { callback_close(); }
    },
    "play_next");

  buttons.emplace_back(
    tr::get("popover.track.append_to_queue"),
    [ti, callback_close]() -> void {
      player::enqueue(ti, player::get_playing_queue().size());

      notifications->push(
        tr::format("notification.appended_to_queue", db::track_by_id(ti.track_id)->get().pretty_name()));
      if (callback_close) { callback_close(); }
    },
    "append_to_queue");

  if (!is_loved) {
    buttons.emplace_back(
      tr::get("popover.track.love"),
      [track_id = ti.track_id, callback_close]() {
        love_track(track_id);
        if (callback_close) { callback_close(); }
      },
      "love_track");
  } else {
    buttons.emplace_back(
      tr::get("popover.track.unlove"),
      [track_id = ti.track_id, callback_close]() {
        unlove_track(track_id);
        if (callback_close) { callback_close(); }
      },
      "unlove_track");
  }

  bool is_album = db::playlist_by_id(ti.playlist_id)->get().type == db::PlaylistType::Album;
  if (!is_album) {
    buttons.emplace_back(
      tr::get("popover.track.show_in_album"),
      [track_id = ti.track_id, callback_close]() -> void {
        auto& track = db::track_by_id(track_id)->get();
        auto album_id = track.originating_album_id;
        if (album_id != db::INVALID_ID) {
          auto collection_id = db::collection_of_playlist(album_id);
          if (collection_id.has_value()) {
            bool immediate = active_collection_id != collection_id;
            show_collection(collection_id.value());
            panel_tracks->scroll_to_track(album_id, track_id, immediate);
            panel_albums->scroll_to_playlist(album_id, immediate);
          }
        }
        if (callback_close) { callback_close(); }
      },
      "show_in_album");
  }

  buttons.emplace_back(
    tr::get("popover.track.add_to_playlist"),
    [track_id = ti.track_id, callback_close]() -> void {
      show_add_to_playlist_popup(track_id);
      if (callback_close) { callback_close(); }
    },
    "add_to_playlist");

  if (remove_from_playlist_option && is_user_playlist && ti.playlist_id != db::playlist_loved_tracks_id()) {
    auto& playlist = db::playlist_by_id(ti.playlist_id)->get();
    buttons.emplace_back(
      tr::format("popover.track.remove_from_playlist", playlist.name),
      [ti, callback_close]() -> void {
        auto& playlist_ = db::playlist_by_id(ti.playlist_id)->get();
        if (ti.index != db::INVALID_ID) {
          ensure(playlist_.track_ids.size() > ti.index);
          ensure(ti.track_id == playlist_.track_ids[ti.index]);
          if (db::remove_track_index_from_playlist(ti.playlist_id, ti.index)) {
            auto track_pretty_name = db::track_by_id(ti.track_id)->get().pretty_name();
            notifications->push(
              tr::format("notification.removed_track_from_playlist", track_pretty_name, playlist_.name));
          }
        } else {
          if (db::remove_track_id_from_playlist(ti.playlist_id, ti.track_id)) {
            auto track_pretty_name = db::track_by_id(ti.track_id)->get().pretty_name();
            notifications->push(
              tr::format("notification.removed_track_from_playlist", track_pretty_name, playlist_.name));
          }
        }
        panel_tracks->recreate(active_collection_id);
        if (callback_close) { callback_close(); }
      },
      "remove_from_playlist");
  }

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "playlist_track_actions",
    .title = "",
    .at = at,
    .distance = 4,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
};

static void show_popover_tracklist_tracks_actions(WidgetTrack* widget, std::span<const db::track_info> tracks,
                                                  const std::function<void()>& callback_close) {
  if (tracks.empty()) { return; }

  bool all_tracks_loved = true;
  bool all_tracks_not_loved = true;
  std::optional<db::playlist_id_t> common_playlist_id = tracks.begin()->playlist_id;
  for (auto& track_info : tracks) {
    if (track_info.playlist_id != common_playlist_id) { common_playlist_id = std::nullopt; }
    bool is_loved = db::playlist_loved_tracks().has_track_id(track_info.track_id);
    if (!is_loved && all_tracks_loved) { all_tracks_loved = false; }
    if (is_loved && all_tracks_not_loved) { all_tracks_not_loved = false; }
  }

  bool is_user_playlist = common_playlist_id.has_value() &&
                          db::playlist_by_id(common_playlist_id.value()).value().get().type == db::PlaylistType::User;
  // bool is_album = common_playlist_id.has_value() &&
  //                 db::playlist_by_id(common_playlist_id.value()).value().get().type == db::PlaylistType::Album;

  std::vector<db::track_id_t> track_ids;
  std::vector<size_t> track_indices;
  track_ids.reserve(tracks.size());
  track_indices.reserve(tracks.size());
  for (const auto& ti : tracks) {
    track_ids.emplace_back(ti.track_id);
    track_indices.emplace_back(ti.index);
  }

  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("popover.track.play_plural"),
    [tracks_ = std::vector(tracks.begin(), tracks.end()), callback_close]() -> void {
      player::clear_queue();
      player::add_to_queue(tracks_, 0);
      player::set_playing_index(0);
      if (callback_close) { callback_close(); }
    },
    "play_track");

  buttons.emplace_back(
    tr::get("popover.track.play_next_plural"),
    [tracks_ = std::vector(tracks.begin(), tracks.end()), callback_close]() -> void {
      player::add_to_queue(tracks_, player::get_playing_index().value_or(player::get_playing_queue().size()) + 1);
      notifications->push(tr::format("notification.appended_to_queue_plural", tracks_.size()));
      if (callback_close) { callback_close(); }
    },
    "play_next");

  buttons.emplace_back(
    tr::get("popover.track.append_to_queue_plural"),
    [tracks_ = std::vector(tracks.begin(), tracks.end()), callback_close]() -> void {
      player::add_to_queue(tracks_, player::get_playing_queue().size());
      notifications->push(tr::format("notification.appended_to_queue_plural", tracks_.size()));
      if (callback_close) { callback_close(); }
    },
    "append_to_queue");

  if (!all_tracks_loved) {
    buttons.emplace_back(
      tr::get("popover.track.love"),
      [track_ids, callback_close]() -> void {
        love_tracks(track_ids);
        if (callback_close) { callback_close(); }
      },
      "love_track");
  }

  if (!all_tracks_not_loved) {
    buttons.emplace_back(
      tr::get("popover.track.unlove"),
      [track_ids, callback_close]() -> void {
        unlove_tracks(track_ids);
        if (callback_close) { callback_close(); }
      },
      "unlove_track");
  }

  buttons.emplace_back(
    tr::get("popover.track.add_to_playlist"),
    [track_ids, callback_close]() -> void {
      show_add_to_playlist_popup(track_ids);
      if (callback_close) { callback_close(); }
    },
    "add_to_playlist");

  if (common_playlist_id.has_value() && is_user_playlist && common_playlist_id != db::playlist_loved_tracks_id()) {
    auto& playlist = db::playlist_by_id(common_playlist_id.value())->get();
    buttons.emplace_back(
      tr::format("popover.track.remove_from_playlist", playlist.name),
      [track_indices, callback_close, common_playlist_id]() -> void {
        auto& playlist_ = db::playlist_by_id(common_playlist_id.value())->get();
        db::remove_track_indices_from_playlist(common_playlist_id.value(), track_indices);
        if (track_indices.size() != 0) {
          notifications->push(
            tr::format("notification.removed_tracks_from_playlist", track_indices.size(), playlist_.name));
          panel_tracks->recreate(active_collection_id);
        }
        if (callback_close) { callback_close(); }
      },
      "remove_from_playlist");
  }

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "playlist_tracks_actions",
    .title = tr::format("popover.track.title_plural", track_ids.size()),
    .at = at,
    .distance = 4,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popover_collection_actions(db::collection_id_t collection_id, Widget* widget) {
  if (collection_id == 0) { return; }
  vec2i at = widget->get_position(Anchor::CENTER);
  decltype(popover_descriptor::buttons) buttons;
  buttons.emplace_back(
    tr::get("dialog.action.rename"), [collection_id]() { show_popup_rename_collection(collection_id); },
    "rename_collection");

  buttons.emplace_back(
    tr::get("dialog.action.set_sources"), [collection_id]() { show_popup_set_sources(collection_id); }, "set_sources");

  buttons.emplace_back(
    tr::get("dialog.action.rescan"),
    [collection_id]() {
      for (auto& path : db::collection_by_id(collection_id)->get().paths()) {
        zincbox::scanner::scan_directory(utf8_to_path(path), collection_id);
      }
    },
    "rescan");

  buttons.emplace_back(
    tr::get("dialog.action.delete"), [collection_id]() { show_popup_delete_collection(collection_id); },
    "delete_collection");

  popover_descriptor d{
    .id = "collection_actions",
    .title = "",
    .at = at,
    .distance = 10,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
};

static void show_popover_queue_track_actions(db::track_info ti, WidgetTrack* widget) {
  auto play = player::get_playing_queue()[ti.index];
  size_t track_id = play.track_id;
  size_t playlist_id = play.playlist_id;
  size_t collection_id = play.collection_id;
  bool is_loved = db::playlist_loved_tracks().has_track_id(track_id);

  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("popover.queue.remove"),
    [queue_index = ti.index]() {
      player::remove_from_queue(queue_index);
      panel_queue->on_queue_changed();
    },
    "remove_from_queue");

  bool is_album = db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::Album;
  buttons.emplace_back(
    is_album ? tr::get("popover.track.show_in_album") : tr::get("popover.track.show_in_playlist"),
    [collection_id, playlist_id, track_id]() {
      show_collection(collection_id);
      panel_tracks->scroll_to_track(playlist_id, track_id);
    },
    is_album ? "show_in_album" : "show_in_playlist");
  if (!is_album) {
    buttons.emplace_back((tr::get("popover.track.show_in_album")),
                         [track_id]() -> void {
                           auto& track = db::track_by_id(track_id)->get();
                           auto album_id = track.originating_album_id;
                           if (album_id != db::INVALID_ID) {
                             auto collection_id_ = db::collection_of_playlist(album_id);
                             if (collection_id_.has_value()) {
                               show_collection(collection_id_.value());
                               panel_tracks->scroll_to_track(album_id, track_id);
                             }
                           }
                         },
                         "show_in_album");
  }

  if (!is_loved) {
    buttons.emplace_back(
      tr::get("popover.track.love"),
      [track_id, queue_index = ti.index]() -> void {
        love_track(track_id);
        panel_queue->on_queue_changed_at(queue_index);
      },
      "love_track");
  } else {
    buttons.emplace_back(
      tr::get("popover.track.unlove"),
      [track_id, queue_index = ti.index]() -> void {
        unlove_track(track_id);
        panel_queue->on_queue_changed_at(queue_index);
      },
      "unlove_track");
  }

  buttons.emplace_back((tr::get("popover.track.add_to_playlist")),
                       [track_id]() -> void { show_add_to_playlist_popup(track_id); }, "add_to_playlist");

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "playlist_track_actions",
    .title = "",
    .at = at,
    .distance = 4,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
};

static void show_popover_queue_tracks_actions(WidgetTrack* widget, std::span<const db::track_info> tracks,
                                              const std::function<void()>& callback_close) {
  if (tracks.empty()) { return; }

  bool all_tracks_loved = true;
  bool all_tracks_not_loved = true;

  std::vector<db::track_id_t> track_ids;
  std::vector<size_t> queue_indices;
  track_ids.reserve(tracks.size());
  queue_indices.reserve(tracks.size());

  for (const auto& track_info : tracks) {
    track_ids.push_back(track_info.track_id);
    queue_indices.push_back(track_info.index);

    bool is_loved = db::playlist_loved_tracks().has_track_id(track_info.track_id);
    if (!is_loved) { all_tracks_loved = false; }
    if (is_loved) { all_tracks_not_loved = false; }
  }

  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("popover.track.play_next_plural"),
    [tracks_ = std::vector(tracks.begin(), tracks.end()), callback_close]() -> void {
      auto playing_index = player::get_playing_index();
      if (playing_index.has_value()) {
        size_t target_index = playing_index.value();
        size_t adjusted_target_index = target_index;

        std::vector<size_t> indices_to_remove;
        indices_to_remove.reserve(tracks_.size());

        for (const auto& item : tracks_) {
          indices_to_remove.push_back(item.index);
          if (item.index <= target_index) { adjusted_target_index -= 1; }
        }

        player::remove_from_queue(indices_to_remove);
        player::add_to_queue(tracks_, adjusted_target_index + 1);
      } else {
        std::vector<size_t> indices_to_remove;
        indices_to_remove.reserve(tracks_.size());
        for (const auto& item : tracks_) {
          indices_to_remove.push_back(item.index);
        }
        player::remove_from_queue(indices_to_remove);
        player::add_to_queue(tracks_, 0);
      }

      panel_queue->on_queue_changed();
      if (callback_close) { callback_close(); }
    },
    "play_next");

  buttons.emplace_back(
    tr::get("popover.queue.remove_plural"),
    [queue_indices, callback_close]() -> void {
      player::remove_from_queue(queue_indices);
      panel_queue->on_queue_changed();
      if (callback_close) { callback_close(); }
    },
    "remove_from_queue");

  if (!all_tracks_loved) {
    buttons.emplace_back(
      tr::get("popover.track.love"),
      [track_ids, callback_close]() -> void {
        love_tracks(track_ids);
        panel_queue->on_queue_changed();
        if (callback_close) { callback_close(); }
      },
      "love_track");
  }

  if (!all_tracks_not_loved) {
    buttons.emplace_back(
      tr::get("popover.track.unlove"),
      [track_ids, callback_close]() -> void {
        unlove_tracks(track_ids);
        panel_queue->on_queue_changed();
        if (callback_close) { callback_close(); }
      },
      "unlove_track");
  }

  buttons.emplace_back(
    tr::get("popover.track.add_to_playlist"),
    [track_ids, callback_close]() -> void {
      show_add_to_playlist_popup(track_ids);
      if (callback_close) { callback_close(); }
    },
    "add_to_playlist");

  vec2i at = widget->get_position(Anchor::CENTER);
  at.x = Input::get_mouse_x();
  popover_descriptor d{
    .id = "queue_tracks_actions",
    .title = tr::format("popover.track.title_plural", tracks.size()),
    .at = at,
    .distance = 4,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popover_playlist_actions(db::playlist_id_t playlist_id, Widget* widget, bool play_actions,
                                          const std::function<void()>& callback_close) {
  decltype(popover_descriptor::buttons) buttons;

  if (play_actions) {
    buttons.emplace_back((tr::get("popover.playlist.play")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           if (panel_albums->get_collection_id().has_value()) {
                             player::play_playlist(*(panel_albums->get_collection_id()), playlist_id, true);
                           }
                         },
                         "play_track");

    buttons.emplace_back((tr::get("popover.playlist.play_next")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           if (panel_albums->get_collection_id().has_value()) {
                             if (player::play_playlist(*(panel_albums->get_collection_id()), playlist_id, false)) {
                               notifications->push(tr::format("notification.appended_to_queue",
                                                              db::playlist_by_id(playlist_id)->get().name));
                             }
                           }
                         },
                         "play_next");
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album && playlist_id != 0) {
    buttons.emplace_back((tr::get("dialog.action.rename")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           show_popup_rename_playlist(playlist_id);
                         },
                         "rename_playlist");
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album) {
    buttons.emplace_back((tr::get("dialog.action.save_as_json")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           auto& playlist = db::playlist_by_id(playlist_id)->get();
                           std::string json;
                           auto ec = glz::write<glz::opts{.prettify = true}>(PlaylistSerialized(playlist), json);
                           if (ec) {
                             return; // FIXME: handle errors
                           }
                           NFD::UniquePathU8 outPath;
                           std::string json_filter_label = tr::get("dialog.filter.json_files");
                           nfdfilteritem_t filterList[1] = {{json_filter_label.c_str(), "json"}};
                           auto file_name_utf8 = playlist.name + ".json";
                           const nfdu8char_t* defaultName = file_name_utf8.c_str();
                           auto result = NFD::SaveDialog(outPath, filterList, 1, nullptr, defaultName);
                           if (result == NFD_OKAY) {
                             nfdu8char_t* path = outPath.get();
                             std::string path_str(path);
                             std::ofstream out(path_str);
                             out << json;
                             out.flush();
                             out.close();
                           } else {
                             // FIXME: handle errors
                           }
                         },
                         "save_playlist_as_json");
  }

  buttons.emplace_back((tr::get("dialog.action.pick_image_file")),
                       [callback_close, playlist_id]() -> void {
                         if (callback_close) { callback_close(); }
                         NFD::UniquePathU8 out_path_n;

                         std::string image_filter_label = tr::get("dialog.filter.image_files");
                         nfdfilteritem_t filter_item[1] = {{image_filter_label.c_str(), "png,jpg,jpeg"}};
                         auto result = NFD::OpenDialog(out_path_n, filter_item, 1);

                         if (result == NFD_OKAY) {
                           nfdu8char_t* path = out_path_n.get();
                           std::string path_str(path);
                           db::set_playlist_image(playlist_id, path_str);
                           std::string playlist_id_str = std::to_string(playlist_id);
                           root->get_texture_atlas().remove_texture(playlist_id_str);
                           root->get_texture_atlas().add_texture(
                             playlist_id_str, db::playlist_by_id(playlist_id)->get().art_64x64, 64, 64);
                           panel_albums->recreate();
                         }
                       },
                       "pick_playlist_cover");

  if (!db::playlist_by_id(playlist_id)->get().art_64x64.empty()) {
    buttons.emplace_back((tr::get("dialog.action.reset_image")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           db::reset_playlist_image(playlist_id);
                           std::string playlist_id_str = std::to_string(playlist_id);
                           root->get_texture_atlas().remove_texture(playlist_id_str);
                           root->get_texture_atlas().add_texture_alias(playlist_id_str, "cover_unknown");
                           panel_albums->recreate();
                         },
                         "reset_playlist_cover");
  }

  if (db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::Album) {
    buttons.emplace_back((tr::get("dialog.action.show_directory")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           auto& playlist = db::playlist_by_id(playlist_id)->get();
                           if (playlist.get_tracks_count() > 0) {
                             auto& track = db::track_by_id(playlist.get_track_ids()[0])->get();
                             io::open_folder_in_file_manager(track.file_path.parent_path());
                           }
                         },
                         "show_playlist_directory");
  }

  if (db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::Album && playlist_id != 0) {
    buttons.emplace_back((tr::get("dialog.action.remove")),
                         [callback_close, playlist_id]() -> void {
                           if (callback_close) { callback_close(); }
                           show_popup_delete_playlist(playlist_id);
                         },
                         "delete_playlist");
  }

  vec2i at = widget->get_position(Anchor::CENTER);
  popover_descriptor d{
    .id = "playlist_actions",
    .title = "",
    .at = at,
    .distance = 16,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popover_playlist_sort_options(db::playlist_id_t playlist_id, Widget* widget) {
  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("sort.artist.name.asc"),
    [playlist_id]() -> void {
      db::sort_playlist_by_artist_asc(playlist_id);
      panel_albums->recreate();
      panel_tracks->recreate(active_collection_id);
    },
    "artist_asc");

  buttons.emplace_back(
    tr::get("sort.artist.name.desc"),
    [playlist_id]() -> void {
      db::sort_playlist_by_artist_desc(playlist_id);
      panel_albums->recreate();
      panel_tracks->recreate(active_collection_id);
    },
    "artist_desc");

  buttons.emplace_back(
    tr::get("sort.title.name.asc"),
    [playlist_id]() -> void {
      db::sort_playlist_by_name_asc(playlist_id);
      panel_albums->recreate();
      panel_tracks->recreate(active_collection_id);
    },
    "title_asc");

  buttons.emplace_back(
    tr::get("sort.title.name.desc"),
    [playlist_id]() -> void {
      db::sort_playlist_by_name_desc(playlist_id);
      panel_albums->recreate();
      panel_tracks->recreate(active_collection_id);
    },
    "title_desc");

  vec2i at = widget->get_position(Anchor::CENTER);
  popover_descriptor d{
    .id = "playlist_sort",
    .title = "",
    .at = at,
    .distance = 16,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popover_create_playlist(Widget* w) {
  decltype(popover_descriptor::buttons) buttons;
  buttons.emplace_back((tr::get("popover.new_playlist.add")), []() { show_popup_new_playlist(); }, "add_playlist");
  buttons.emplace_back((tr::get("popover.new_playlist.add_smart")), show_popup_new_smart_playlist,
                       "add_smart_playlist");
  buttons.emplace_back((tr::get("popover.new_playlist.add_from_json")), show_dialog_new_playlist_from_json,
                       "add_playlist_from_json");

  popover_descriptor d{
    .id = "new_playlist",
    .title = "",
    .at = w->get_position(Anchor::CENTER),
    .distance = 16,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popover_queue_actions(Widget* w) {
  decltype(popover_descriptor::buttons) buttons;

  buttons.emplace_back(
    tr::get("popover.queue.clear"),
    []() -> void {
      player::clear_queue();
      panel_queue->recreate();
    },
    "clear_queue");

  buttons.emplace_back(
    tr::get("popover.queue.save_as_playlist"),
    []() -> void {
      auto* popup = popup_controller->show_popup<PopupInput>();
      popup->set_size(300, 200);
      popup->title->set_text(tr::get("popup.playlist.create_from_queue.title"));
      popup->btn_ok->get_label().set_text(tr::get("dialog.action.add"));
      popup->text_input->set_focused(true);

      popup->on_ok_pressed = [popup]() {
        auto playlist_id = db::add_playlist_to_collection(
          0, db::Playlist{popup->text_input->label.get_text(), {""}, db::PlaylistType::User});
        for (const auto& play : player::get_playing_queue()) {
          db::add_track_id_to_playlist(playlist_id, play.track_id);
        }
        if (active_collection_id == 0) {
          panel_albums->props.collection_id = 0;
          panel_albums->recreate();
          panel_tracks->recreate(active_collection_id);
        }
        notifications->push(tr::format("notification.saved_queue_as_playlist", popup->text_input->label.get_text()));
      };

      popup->on_cancel_pressed = []() {};
    },
    "save_queue_as_playlist");

  popover_descriptor d{
    .id = "queue_actions",
    .title = "",
    .at = w->get_position(Anchor::CENTER),
    .distance = 10,
    .buttons = buttons,
  };
  popup_controller->create_popover(d);
}

static void show_popup_new_playlist(const std::function<void(std::optional<db::playlist_id_t>)>& callback_close) {
  auto* popup = popup_controller->show_popup<PopupInput>();
  popup->set_size(300, 200);
  popup->title->set_text(tr::get("popup.playlist.add.title"));
  popup->btn_ok->get_label().set_text(tr::get("dialog.action.add"));
  popup->text_input->set_focused(true);

  popup->on_ok_pressed = [popup, callback_close]() {
    auto playlist_id = db::add_playlist_to_collection(
      0, db::Playlist{popup->text_input->label.get_text(), {""}, db::PlaylistType::User});
    if (active_collection_id == 0) {
      panel_albums->props.collection_id = 0;
      panel_albums->recreate();
      panel_tracks->recreate(active_collection_id);
    }
    if (callback_close) { callback_close(playlist_id); }
  };

  popup->on_cancel_pressed = [callback_close]() {
    if (callback_close) { callback_close(std::nullopt); }
  };
}

static void show_popup_new_smart_playlist() { popup_controller->show_popup<PopupCreateSmartPlaylist>(); }

static void show_dialog_new_playlist_from_json() {
  NFD::UniquePathU8 result;
  std::string json_filter_label = tr::get("dialog.filter.json_files");
  nfdfilteritem_t filterList[1] = {{json_filter_label.c_str(), "json"}};
  nfdresult_t res = NFD::OpenDialog(result, filterList, 1);
  if (res == NFD_OKAY) {}
}

static void add_track_to_playlist(db::playlist_id_t playlist_id, db::track_id_t track_id) {
  bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
  if (db::add_track_id_to_playlist(playlist_id, track_id)) {
    auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
    if (loved_tracks) {
      notifications->push(tr::format("notification.loved_track", track_pretty_name));
      if (player::get_playing().has_value() && track_id == player::get_playing()->track_id) {
        panel_controls->update_love_state(true);
      }
    } else {
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.added_track_to_playlist", track_pretty_name, playlist_name));
    }
  }
  if (active_collection_id.has_value()) {
    panel_tracks->recreate(active_collection_id);
  } else {
    panel_queue->recreate();
  }
}

static void add_tracks_to_playlist(db::playlist_id_t playlist_id, std::span<const db::track_id_t> track_ids) {
  bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
  i32 added_count = 0;
  for (db::track_id_t track_id : track_ids) {
    if (db::add_track_id_to_playlist(playlist_id, track_id)) {
      added_count += 1;
      if (loved_tracks && player::get_playing().has_value() && track_id == player::get_playing()->track_id) {
        panel_controls->update_love_state(true);
      }
    }
  }
  if (added_count != 0) {
    if (loved_tracks) {
      notifications->push(tr::format("notification.loved_tracks", added_count));
    } else {
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.added_tracks_to_playlist", added_count, playlist_name));
    }
    if (active_collection_id.has_value()) { panel_tracks->recreate(active_collection_id); }
  }
}

static void remove_track_from_playlist(db::playlist_id_t playlist_id, db::track_id_t track_id) {
  bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
  if (db::remove_track_id_from_playlist(playlist_id, track_id)) {
    auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
    if (loved_tracks) {
      notifications->push(tr::format("notification.unloved_track", track_pretty_name));
      if (player::get_playing().has_value() && track_id == player::get_playing()->track_id) {
        panel_controls->update_love_state(false);
      }
    } else {
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.removed_track_from_playlist", track_pretty_name, playlist_name));
    }
  }
  if (active_collection_id.has_value()) {
    panel_tracks->recreate(active_collection_id);
  } else {
    panel_queue->recreate();
  }
}

static void remove_tracks_from_playlist(db::playlist_id_t playlist_id, std::span<const db::track_id_t> track_ids) {
  bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
  i32 removed_count = 0;
  for (db::track_id_t track_id : track_ids) {
    if (db::remove_track_id_from_playlist(playlist_id, track_id)) {
      removed_count += 1;
      if (loved_tracks && player::get_playing().has_value() && track_id == player::get_playing()->track_id) {
        panel_controls->update_love_state(false);
      }
    }
  }
  if (removed_count != 0) {
    if (loved_tracks) {
      notifications->push(tr::format("notification.unloved_tracks", removed_count));
    } else {
      auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
      notifications->push(tr::format("notification.removed_tracks_from_playlist", removed_count, playlist_name));
    }

    if (active_collection_id.has_value()) { panel_tracks->recreate(active_collection_id); }
  }
}

static void love_track(db::track_id_t track_id) { add_track_to_playlist(db::playlist_loved_tracks_id(), track_id); }

static void love_tracks(std::span<const db::track_id_t> track_ids) {
  add_tracks_to_playlist(db::playlist_loved_tracks_id(), track_ids);
}

static void unlove_track(db::track_id_t track_id) {
  remove_track_from_playlist(db::playlist_loved_tracks_id(), track_id);
}

static void unlove_tracks(std::span<const db::track_id_t> track_ids) {
  remove_tracks_from_playlist(db::playlist_loved_tracks_id(), track_ids);
}

static void show_search_popup() {
  if (mini_player.value_or(false)) { return; }
  if (search_popup_visible) { return; }
  auto* popup = popup_controller->show_popup<PopupSearch>();
  auto callback_close = [popup]() -> void {
    popup->close();
    if (popup->on_closed) { popup->on_closed(); }
  };
  popup->set_collection_id(active_collection_id.value_or(0));
  search_popup_visible = true;
  popup->on_playlist_lmb = [popup](size_t playlist_id, Widget*) -> void {
    auto collection_id = db::collection_of_playlist(playlist_id);
    if (collection_id.has_value()) {
      show_collection(collection_id.value());
      panel_tracks->scroll_to_playlist(playlist_id);
      popup->close();
      popup->on_closed();
    }
  };

  popup->on_track_lmb = [popup](db::track_info ti, Widget*) {
    show_collection(ti.collection_id);
    panel_tracks->scroll_to_track(ti.playlist_id, ti.track_id);
    popup->close();
    popup->on_closed();
  };

  popup->on_playlist_rmb = [callback_close](size_t playlist_id, Widget* w) -> void {
    show_popover_playlist_actions(playlist_id, w, true, callback_close);
  };

  popup->on_track_rmb = [callback_close](db::track_info ti, Widget* w) -> void {
    show_popover_tracklist_track_actions(ti, w, false, callback_close);
  };

  popup->on_closed = []() { search_popup_visible = false; };
}

static void show_settings_popup() {
  auto* popup = popup_controller->show_popup<PopupSettings>();
  popup->on_save = [](Settings new_settings) -> void {
    if (zincbox::settings().must_reload(new_settings)) {
      auto* popup_close = popup_controller->show_popup<PopupConfirm>(tr::get("popup.reload_required.content"));
      popup_close->title->set_text(tr::get("popup.reload_required.title"));
      popup_close->content->update();
      popup_close->set_width(popup_close->content->get_width() + 32);
      popup_close->btn_ok->get_label().set_text(tr::get("dialog.action.reload"));
      popup_close->btn_cancel->get_label().set_text(tr::get("dialog.action.cancel"));
      popup_close->on_ok_pressed = []() { zincbox::stop(); };
    }
    zincbox::settings() = new_settings;
  };
}

static void show_about_popup() { popup_controller->show_popup<PopupAbout>(); }

static void quit() { zincbox::stop(); }

bool zincbox::ui::get_mini_player() { return mini_player.value_or(false); }

std::string zincbox::ui::get_selected_tab() {
  const Tab* selected_tab = panel_top->get_tab_bar()->get_selected_tab();
  if (!selected_tab) { return ""; }
  return selected_tab->get_label().get_text();
}

std::vector<std::string> zincbox::ui::get_tabs_order() {
  std::vector<std::string> ret;
  for (Tab* tab : panel_top->get_tab_bar()->get_tabs()) {
    ret.emplace_back(tab->get_label().get_text());
  }
  return ret;
}

i32 zincbox::ui::get_tracks_scroll_offset() { return panel_tracks->get_scroll_px(); }
i32 zincbox::ui::get_playlists_scroll_offset() { return panel_albums->get_scroll_px(); }

void zincbox::ui::set_mini_player(bool state) {
  if (mini_player == state) { return; }
  mini_player = state;
  if (mini_player.value()) {
    panel_tracks->hide();
    panel_albums->hide();
    panel_queue->hide();
    splitter->set_is_drawn(false);
    splitter->set_is_updated(false);
    panel_top->set_is_updated(false);
    panel_top->set_is_drawn(false);
  } else {
    auto active_collection_id_ = active_collection_id;
    active_collection_id = std::nullopt;
    if (active_collection_id_.has_value()) {
      show_collection(active_collection_id_.value());
    } else {
      show_queue();
    }
    panel_top->set_is_updated(true);
    panel_top->set_is_drawn(true);
  }

  panel_controls->set_button_expand_player_visibility(mini_player.value_or(false));
  panel_controls->set_tooltip_visibility(!mini_player.value_or(false));
}

void zincbox::ui::set_selected_tab(std::string tab_label) {
  auto* tab = panel_top->get_tab_bar()->get_tab_by_label(tab_label);
  if (!tab) { return; }
  panel_top->get_tab_bar()->select_tab(tab->id);
}

void zincbox::ui::set_tabs_order(std::vector<std::string> tabs_order_) {
  tabs_order = std::move(tabs_order_);
  panel_top->get_tab_bar()->sort_tabs_by_label(tabs_order);
}

void zincbox::ui::set_tracks_scroll_offset(i32 scroll_px) { panel_tracks->set_scroll_px(scroll_px); }

void zincbox::ui::set_playlists_scroll_offset(i32 scroll_px) { panel_albums->set_scroll_px(scroll_px); }
