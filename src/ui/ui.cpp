#include "ui.hpp"
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
#include <variant>
#include <vector>
#include <glaze/glaze.hpp>
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
#include "core/player/player.hpp"
#include "core/queue_element.hpp"
#include "core/scanner.hpp"
#include "core/settings.hpp"
#include "ui/cover_art.hpp"
#include "ui/file_dialog.hpp"
#include "ui/panel_controls.hpp"
#include "ui/panel_playlists.hpp"
#include "ui/panel_queue.hpp"
#include "ui/panel_top.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/popup_definitions.hpp"
#include "ui/popup_search.hpp"
#include "ui/popup_settings.hpp"
#include "ui/sdl3_window.hpp"
#include "ui/splitter.hpp"
#include "ui/tab_bar.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/widget_playlist_header.hpp"
#include "ui/widget_queue_playlist_collection.hpp"
#include "ui/widget_track.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/widget.hpp"
#include "ui_notifications.hpp"
#include "zincbox.hpp"

namespace fs = std::filesystem;
using namespace zincgui;

namespace {
  std::optional<size_t> active_collection_id;
  std::vector<float> tracks_scroll_positions;
  std::vector<float> playlists_scroll_positions;
  std::vector<std::string> tabs_order;
  bool search_popup_visible = false;
  std::optional<bool> mini_player = std::nullopt;

  enum class SelectionSource : u8 {
    COLLECTION,
    QUEUE,
  };

  struct {
      std::vector<PanelTracklist::Item> items;
      std::vector<PanelTracklist::Item> items_sorted;
      std::optional<db::playlist_id_t> common_playlist_id;
      vec2i start{};
      bool started = false;
      i32 drag_tab_id = -1;
      i32 drag_tab_timer = 0;
      SelectionSource source;
  } selection;

  constexpr size_t SELECTION_DRAG_TAB_TIMER = 15;

  std::unique_ptr<Root> root;
  class ShortcutInterceptor* shortcut_interceptor{};
  PopupController* popup_controller{};
  InterfaceNotifications* notifications{};
  ColorRect* bg{};
  PanelTop* panel_top{};
  PanelTracklist* panel_tracklist{};
  PanelQueue* panel_queue{};
  PanelPlaylists* panel_playlists{};
  PanelControls* panel_controls{};
  Splitter* splitter{};
  ToolTip* tooltip_drag{};
  Notification* notification_scan_progress{};

  enum class FileDialogOpen : u8 {
    NONE,
    CREATE_COLLECTION,
    ADD_PATH_TO_COLLECTION,
    SAVE_JSON,
    PICK_PLAYLIST_IMAGE
  } file_dialog_type = FileDialogOpen::NONE;

  std::future<std::vector<std::filesystem::path>> file_dialog_future_path;
  std::variant<size_t, std::string, std::monostate> file_dialog_data = std::monostate{};

  void recreate_panel_top(bool order = true);
  void init_atlas();
  void handle_dropped_files();
  void handle_drag_and_drop();
  void delete_collection(db::collection_id_t);
  void delete_playlist(size_t);
  void show_collection(db::collection_id_t);
  void show_queue();
  void show_add_to_playlist_popup(db::track_id_t);
  void show_add_to_playlist_popup(std::span<const db::track_id_t>);
  void show_popup_delete_collection(db::collection_id_t);
  void show_popup_rename_collection(db::collection_id_t);
  void show_popup_set_sources(db::collection_id_t);
  void show_popup_delete_playlist(db::playlist_id_t);
  void show_popup_rename_playlist(db::playlist_id_t);
  void show_popover_tracklist_track_actions(db::track_info, Widget*, bool remove_from_playlist_option,
                                            const std::function<void()>& callback_close = nullptr);
  void show_popover_tracklist_tracks_actions(size_t item_index, const std::function<void()>& callback_close = nullptr);
  void show_popover_collection_actions(db::collection_id_t, Widget*);
  void show_popover_queue_item_actions(size_t);
  void show_popover_queue_items_actions(size_t, const std::function<void()>& callback_close = nullptr);
  void show_popover_playlist_actions(db::playlist_id_t, Widget*, bool play_actions = true,
                                     const std::function<void()>& callback_close = nullptr);
  void show_popover_playlist_sort_options(db::playlist_id_t, Widget*);
  void show_popover_create_playlist(Widget*);
  void show_popover_queue_actions(Widget*);
  void show_popup_new_playlist(const std::function<void(std::optional<db::playlist_id_t>)>& callback_close = nullptr);
  void show_popup_new_smart_playlist();
  void show_dialog_new_playlist_from_json();
  void add_track_to_playlist(db::playlist_id_t, db::track_id_t);
  void add_tracks_to_playlist(db::playlist_id_t, std::span<const db::track_id_t>);
  void remove_track_from_playlist(db::playlist_id_t, db::track_id_t);
  void remove_tracks_from_playlist(db::playlist_id_t, std::span<const db::track_id_t>);
  void love_track(db::track_id_t);
  void love_tracks(std::span<const db::track_id_t>);
  void unlove_track(db::track_id_t);
  void unlove_tracks(std::span<const db::track_id_t>);
  void show_search_popup();
  void show_settings_popup();
  void show_about_popup();
  void quit();

  class ShortcutInterceptor : public Widget {
    public:
      ShortcutInterceptor(Root& ui_) : Widget(ui_) {}

      void event(Input::InputEventKey& ev) override {
        if (ev.action != Input::KeyAction::RELEASE) { return; }
        bool ctrl =
          Input::key_pressed(Input::Key::KEY_LEFT_CONTROL) || Input::key_pressed(Input::Key::KEY_RIGHT_CONTROL);
        // bool shift = Input::key_pressed(Input::Key::KEY_LEFT_SHIFT) ||
        //              Input::key_pressed(Input::Key::KEY_RIGHT_SHIFT);
        if (ctrl && ev.key == Input::Key::KEY_F && search_popup_invoked) {
          ev.handled = true;
          search_popup_invoked();
        }
      }

    public:
      std::function<void(void)> search_popup_invoked;
  };
} // namespace

void zincbox::ui::init() {
  root = std::make_unique<Root>(1, 1);
  std::string language = zincbox::settings().interface.language;
  std::string theme = zincbox::settings().interface.theme;
  theme::load_theme(theme, *root, language);
  zincbox::ui::cover_art::init();
  db::set_playlists_collection_name(tr::get("collection.playlists_collection_name"));
  db::set_loved_tracks_playlist_name(tr::get("playlist.loved_tracks_playlist_name"));

  shortcut_interceptor = &root->add_widget<ShortcutInterceptor>();
  shortcut_interceptor->search_popup_invoked = show_search_popup;

  bg = &root->add_widget<ColorRect>(theme::config().panel_top.color);
  panel_controls = &root->add_widget<PanelControls>();
  panel_top = &root->add_widget<PanelTop>();
  panel_tracklist = &root->add_widget<PanelTracklist>();
  panel_queue = &root->add_widget<PanelQueue>();
  splitter = &root->add_widget<Splitter>();
  tooltip_drag = &root->add_widget<ToolTip>("", ToolTipPosition::MANUAL);
  tooltip_drag->set_is_drawn(false);
  tooltip_drag->set_anchor(Anchor::TOP);
  tooltip_drag->set_clamp(false);
  panel_playlists = &root->add_widget<PanelPlaylists>();
  popup_controller = &root->add_widget<PopupController>();
  notifications = &root->add_widget<InterfaceNotifications>();

  panel_queue->hide();
  panel_tracklist->hide();
  panel_playlists->hide();
  splitter->set_is_updated(false);
  splitter->set_is_drawn(false);

  if (db::track_count() == 0) { popup_controller->show_popup<PopupWelcome>(); }

  panel_controls->on_playing_track_lmb = [](Widget*) -> void {
    if (mini_player.value_or(false)) { return; }
    auto playing = player::get_playing();
    if (playing.has_value()) {
      if (!playing->track()) { return; }
      auto& ti = playing->track()->track_info;
      bool immediate = active_collection_id != ti.collection_id;
      show_collection(ti.collection_id);
      panel_playlists->scroll_to_playlist(ti.playlist_id, immediate);
      panel_tracklist->scroll_to_track(ti.playlist_id, ti.track_id, immediate);
    }
  };

  panel_controls->on_playing_track_rmb = [](Widget* w) -> void {
    if (mini_player.value_or(false)) { return; }
    auto playing = player::get_playing_track();
    if (playing) { show_popover_tracklist_track_actions(playing.value(), w, false); }
  };

  panel_controls->on_love_button_pressed = []() -> void {
    if (auto ti = player::get_playing_track()) {
      if (db::playlist_by_id(0)->get().has_track_id(ti->track_id)) {
        unlove_track(ti->track_id);
      } else {
        love_track(ti->track_id);
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

  panel_top->on_add_collection_button_pressed = [&](Widget*) -> void {
    if (file_dialog_type != FileDialogOpen::NONE) { return; }
    file_dialog_type = FileDialogOpen::CREATE_COLLECTION;
    file_dialog_data = std::monostate{};
    file_dialog_future_path = file_dialog::pick_folder_multiple();
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

  panel_tracklist->on_item_lmb = [&](size_t item_index) -> void {
    auto& item = panel_tracklist->get_item(item_index);
    if (auto* item_track = std::get_if<PanelTracklist::ItemTrack>(&item)) {
      bool playback_error = !player::play(item_track->track_info, true);
      item_track->widget->set_playback_error(playback_error);
    }
  };

  panel_queue->on_item_lmb([&](size_t item_index) -> void {
    auto& item = panel_queue->get_item(item_index);
    if (auto* item_track = std::get_if<PanelTracklist::ItemQueueTrack>(&item)) {
      player::set_playing_index(item_track->queue_index);
    } else if (auto* item_playlist = std::get_if<PanelTracklist::ItemQueuePlaylist>(&item)) {
      player::set_playing_index(item_playlist->queue_index);
    } else if (auto* item_collection = std::get_if<PanelTracklist::ItemQueueCollection>(&item)) {
      player::set_playing_index(item_collection->queue_index);
    }
  });

  panel_queue->on_item_rmb(show_popover_queue_item_actions);
  panel_queue->on_selection_rmb([](size_t item_index) -> void { show_popover_queue_items_actions(item_index); });
  panel_tracklist->on_item_rmb = [](size_t item_index) -> void {
    if (auto* item_track = std::get_if<PanelTracklist::ItemTrack>(&panel_tracklist->get_item(item_index))) {
      show_popover_tracklist_track_actions(item_track->track_info, item_track->widget, true);
    }
  };

  panel_tracklist->on_selection_rmb = [](size_t item_index) -> void {
    show_popover_tracklist_tracks_actions(item_index);
  };

  panel_tracklist->on_playlist_sort_button_pressed = [](size_t, size_t playlist_id, Widget* w) {
    show_popover_playlist_sort_options(playlist_id, w);
  };

  panel_tracklist->on_playlist_more_options_invoked = [](size_t, size_t playlist_id, Widget* w) {
    show_popover_playlist_actions(playlist_id, w, false);
  };

  panel_playlists->on_playlist_lmb = [&](size_t playlist_id, Widget*) -> void {
    panel_tracklist->scroll_to_playlist(playlist_id);
  };

  panel_playlists->on_playlist_rmb = [&](size_t playlist_id, Widget* w) -> void {
    show_popover_playlist_actions(playlist_id, w);
  };

  panel_playlists->on_button_sort_by_pressed = [](Widget* w) {
    decltype(popover_descriptor::buttons) buttons;
    if (active_collection_id == 0) {
      buttons.emplace_back(
        tr::get("sort.playlist.name.asc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::NAME_AZ; }, "name_asc");
      buttons.emplace_back(
        tr::get("sort.playlist.name.desc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::NAME_ZA; }, "name_desc");
    } else if (active_collection_id.has_value()) {
      buttons.emplace_back(
        tr::get("sort.artist.name.asc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::AUTHOR_AZ; }, "artist_asc");
      buttons.emplace_back(
        tr::get("sort.artist.name.desc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::AUTHOR_ZA; }, "artist_desc");
      buttons.emplace_back(
        tr::get("sort.album.name.asc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::NAME_AZ; }, "name_asc");
      buttons.emplace_back(
        tr::get("sort.album.name.desc"),
        []() -> void { panel_playlists->props.sort_by = PanelPlaylists::SortBy::NAME_ZA; }, "name_desc");
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

  panel_playlists->on_add_playlist_button_pressed = [&](Widget* w) -> void { show_popover_create_playlist(w); };

  recreate_panel_top(false);
}

void zincbox::ui::deinit() { root = nullptr; }

void zincbox::ui::update(vec2i window_size) {
  bool covers_added = zincbox::ui::cover_art::update();
  if (covers_added) {
    if (panel_playlists->get_is_updated()) {
      panel_playlists->recreate();
      panel_playlists->show();
    }
  }

  auto scan_progress = zincbox::scanner::get_progress();
  if (scan_progress) {
    if (!notification_scan_progress) { notification_scan_progress = notifications->push_persistent(""); }
    auto msg =
      tr::format("notification.scan_progress", scan_progress->files_scanned, scan_progress->directories_scanned);
    notification_scan_progress->label->set_text(msg);
  } else {
    if (notification_scan_progress) {
      notifications->dismiss_after(notification_scan_progress, 10);
      notification_scan_progress = nullptr;
    }
  }

  auto scan_summary = zincbox::scanner::import();
  if (scan_summary) {
    popup_controller->show_popup<PopupScanSummary>(*scan_summary);

    if (active_collection_id.has_value() && active_collection_id.value() == scan_summary->collection_id) {
      panel_tracklist->recreate(active_collection_id);
      panel_playlists->props.collection_id = scan_summary->collection_id;
      panel_playlists->recreate();
    }
  }

  if (file_dialog::is_ready(file_dialog_future_path)) {
    auto paths = file_dialog_future_path.get();

    if (!paths.empty()) {
      switch (file_dialog_type) {
      case FileDialogOpen::CREATE_COLLECTION: {
        std::string collection_name = (tr::get("collection.default_name")) + std::to_string(db::collection_count() + 1);
        auto collection_id = db::add_collection(collection_name);
        bool added = false;

        for (auto&& path : paths) {
          added |= db::add_path_to_collection(collection_id, path);
        }

        if (added) { zincbox::scanner::scan_collection(collection_id); }

        recreate_panel_top();
        notifications->push(tr::format("notification.added_collection", collection_name));
        break;
      }

      case FileDialogOpen::ADD_PATH_TO_COLLECTION: {
        auto collection_id = std::get<db::collection_id_t>(file_dialog_data);
        if (db::collection_count() <= collection_id) { break; }
        if (db::add_path_to_collection(collection_id, paths.front())) {
          zincbox::scanner::scan_collection(collection_id);
        }
        break;
      }

      case FileDialogOpen::SAVE_JSON: {
        const auto& json_data = std::get<std::string>(file_dialog_data);
        std::ofstream out(paths.front());
        if (out.is_open()) {
          out << json_data;
          out.flush();
          out.close();
        } else {
          std::string err_msg = std::generic_category().message(errno);
          notifications->push_error(tr::format("notification.save_playlist_to_json_error", err_msg));
        }
        break;
      }

      case FileDialogOpen::PICK_PLAYLIST_IMAGE: {
        notifications->push_error("Unimplemented");
        break;
      }

      default: {
        break;
      }
      }
    }

    file_dialog_type = FileDialogOpen::NONE;
    file_dialog_data = std::monostate{};
  }

  handle_dropped_files();
  handle_drag_and_drop();

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

  panel_tracklist->set_pos(content_x, content_y);
  panel_tracklist->set_width(width_tracks);
  panel_tracklist->set_height(content_height);

  splitter->set_pos(content_x + width_tracks, content_y);
  splitter->set_height(content_height);

  panel_playlists->set_pos(content_x + width_tracks + splitter->get_width(), content_y);
  panel_playlists->set_width(width_albums);
  panel_playlists->set_height(content_height);

  panel_controls->set_pos(content_x, -border);
  panel_controls->set_width(content_width);
  root->tick(window_size.x, window_size.y);
}

void zincbox::ui::draw() {
  zincbox::ui::cover_art::texture_atlas().bind(2);
  root->draw();
}

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

i32 zincbox::ui::get_tracks_scroll_offset() { return panel_tracklist->get_scroll_px(); }

i32 zincbox::ui::get_playlists_scroll_offset() { return panel_playlists->get_scroll_px(); }

void zincbox::ui::set_mini_player(bool state) {
  if (mini_player == state) { return; }
  mini_player = state;
  if (mini_player.value()) {
    panel_tracklist->hide();
    panel_playlists->hide();
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

void zincbox::ui::set_selected_tab(const std::string& tab_label) {
  auto* tab = panel_top->get_tab_bar()->get_tab_by_label(tab_label);
  if (!tab) { return; }
  panel_top->get_tab_bar()->select_tab(tab->id);
}

void zincbox::ui::set_tabs_order(std::vector<std::string> tabs_order_) {
  tabs_order = std::move(tabs_order_);
  panel_top->get_tab_bar()->sort_tabs_by_label(tabs_order);
}

void zincbox::ui::set_tracks_scroll_offset(i32 scroll_px) { panel_tracklist->set_scroll_px(scroll_px); }

void zincbox::ui::set_playlists_scroll_offset(i32 scroll_px) { panel_playlists->set_scroll_px(scroll_px); }

zincbox::ui::DecorationHover zincbox::ui::get_decoration_hover(i32 mouse_x, i32 mouse_y) {
  vec2i mouse_pos{mouse_x, mouse_y};
  if (!theme::config().custom_window_decoration.enabled) { return DecorationHover::INSIDE; }
  i32 border_size = theme::config().custom_window_decoration.border_size;
  border_size = std::max(border_size, 6);
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

namespace {

  void recreate_panel_top(bool order) {
    if (order) {
      tabs_order.clear();
      for (Tab* tab : panel_top->get_tab_bar()->get_tabs()) {
        tabs_order.push_back(tab->get_label().get_text());
      }
    }
    panel_top->recreate(active_collection_id);
    panel_top->get_tab_bar()->sort_tabs_by_label(tabs_order);
  }

  void create_collection(std::span<const std::filesystem::path> directories) {
    if (directories.size() == 0) { return; }
    if (!std::filesystem::is_directory(directories[0])) { return; }
    std::string collection_name = path_to_utf8(fs::path{directories[0]}.filename());
    auto collection_id = db::add_collection(collection_name);
    bool added = false;
    for (auto& path : directories) {
      added |= db::add_path_to_collection(collection_id, path);
    }
    if (added) { zincbox::scanner::scan_collection(collection_id); }
    recreate_panel_top();
  }

  void create_multiple_collections(std::span<const std::filesystem::path> directories) {
    if (directories.size() == 0) { return; }
    for (auto& path : directories) {
      if (!std::filesystem::is_directory(path)) { continue; }
      std::string collection_name = path_to_utf8(path.filename());
      auto collection_id = db::add_collection(collection_name);
      if (db::add_path_to_collection(collection_id, path)) { zincbox::scanner::scan_collection(collection_id); }
    }
    recreate_panel_top();
  }

  void handle_dropped_files() {
    std::vector<std::filesystem::path> dropped_directories{};
    for (auto& path_utf8 : Input::get_dropped_paths()) {
      auto path = utf8_to_path(path_utf8);
      if (std::filesystem::is_directory(path)) { dropped_directories.emplace_back(path); }
    }

    if (dropped_directories.empty()) { return; }

    auto* popup = popup_controller->show_popup<PopupImportFolders>(dropped_directories);

    popup->on_add_collections_pressed = [](std::span<const std::filesystem::path> dirs) -> void {
      if (dirs.size() > 1) {
        create_multiple_collections(dirs);
      } else {
        create_collection(dirs);
      }
    };

    popup->on_merge_pressed = [](std::span<const std::filesystem::path> dirs) -> void { create_collection(dirs); };
  }

  void delete_collection(db::collection_id_t collection_id) {
    db::mark_collection_as_tombstone(collection_id);
    bool change_view = !active_collection_id || db::collection_by_id(*active_collection_id)->get().is_tombstone();
    if (change_view) {
      active_collection_id = std::nullopt;
      recreate_panel_top();
      panel_playlists->props.collection_id = active_collection_id;
      panel_tracklist->recreate(active_collection_id);
    } else {
      recreate_panel_top();
      panel_playlists->props.collection_id = active_collection_id;
      panel_tracklist->recreate(active_collection_id);
    }
  }

  void delete_playlist(size_t playlist_id) {
    db::mark_playlist_as_tombstone(playlist_id);

    panel_playlists->recreate();
    panel_tracklist->clear();
    panel_tracklist->recreate(active_collection_id);
  }

  void show_collection(db::collection_id_t collection_id) {
    if (collection_id == active_collection_id) { return; }
    if (collection_id >= db::collection_count()) { return; }

    if (tracks_scroll_positions.size() <= collection_id) { tracks_scroll_positions.resize(collection_id + 1, 0.0f); }
    if (playlists_scroll_positions.size() <= collection_id) {
      playlists_scroll_positions.resize(collection_id + 1, 0.0f);
    }

    if (active_collection_id.has_value()) {
      tracks_scroll_positions[active_collection_id.value()] = panel_tracklist->get_scroll_px();
      playlists_scroll_positions[active_collection_id.value()] = panel_playlists->get_scroll_px();
    }

    active_collection_id = collection_id;

    panel_top->select(collection_id);

    panel_tracklist->recreate(active_collection_id);
    panel_tracklist->set_scroll_px(tracks_scroll_positions[collection_id], true);
    panel_tracklist->show();

    panel_playlists->props.collection_id = active_collection_id;
    panel_playlists->recreate();
    panel_playlists->set_scroll_px(playlists_scroll_positions[collection_id], true);
    panel_playlists->show();

    panel_queue->hide();

    splitter->set_is_drawn(true);
    splitter->set_is_updated(true);
  }

  void show_queue() {
    active_collection_id = std::nullopt;
    panel_tracklist->hide();
    panel_playlists->hide();
    panel_queue->show();

    splitter->set_is_drawn(false);
    splitter->set_is_updated(false);

    panel_top->select(panel_top->get_queue_tab()->id);
  }

  void show_add_to_playlist_popup(db::track_id_t track_id) {
    auto* popup = popup_controller->show_popup<PopupAddToPlaylist>(track_id);

    popup->on_playlist_selected = [track_id](size_t playlist_id) -> void {
      if (playlist_id == 0) {
        love_track(track_id);
      } else if (db::add_track_id_to_playlist(playlist_id, track_id)) {
        auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
        auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
        notifications->push(tr::format("notification.added_track_to_playlist", track_pretty_name, playlist_name));
        panel_tracklist->recreate(active_collection_id);
      }
    };
  }

  void show_add_to_playlist_popup(std::span<const db::track_id_t> track_ids_) {
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
        panel_tracklist->recreate(active_collection_id);
      }
    };
  }

  void show_popup_delete_collection(db::collection_id_t collection_id) {
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

  void show_popup_rename_collection(db::collection_id_t collection_id) {
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

  void show_popup_set_sources(db::collection_id_t collection_id) {
    auto* popup = popup_controller->show_popup<PopupSetSources>(collection_id);

    popup->on_remove_path_pressed = [collection_id](const std::filesystem::path& path) -> void {
      if (db::remove_path_from_collection(collection_id, path)) { zincbox::scanner::scan_collection(collection_id); }
      if (active_collection_id.has_value() && active_collection_id.value() == collection_id) {
        panel_tracklist->recreate(active_collection_id);
        panel_playlists->props.collection_id = collection_id;
      }
      popup_controller->close_all_popups();
    };

    popup->on_add_dir_pressed = [collection_id]() -> void {
      if (file_dialog_type != FileDialogOpen::NONE) { return; }
      file_dialog_type = FileDialogOpen::ADD_PATH_TO_COLLECTION;
      file_dialog_data = collection_id;
      file_dialog_future_path = zincbox::ui::file_dialog::pick_folder();
    };
  }

  void show_popup_delete_playlist(db::playlist_id_t playlist_id) {
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

  void show_popup_rename_playlist(db::playlist_id_t playlist_id) {
    auto* popup = popup_controller->show_popup<PopupInput>();
    popup->set_size(300, 200 * 4);
    popup->title->set_text(tr::get("popup.playlist.rename.title"));
    popup->btn_ok->get_label().set_text(tr::get("dialog.action.rename"));
    popup->text_input->label.set_text(db::playlist_by_id(playlist_id)->get().name);
    popup->text_input->set_focused(true);

    popup->on_ok_pressed = [popup, playlist_id]() {
      std::string new_name = popup->text_input->label.get_text();
      if (new_name.empty()) { return; }
      db::rename_playlist(playlist_id, new_name);
      notifications->push(tr::format("notification.renamed_playlist", new_name));
      panel_playlists->recreate();
    };
  }

  void show_popover_tracklist_track_actions(db::track_info ti, Widget* widget, bool remove_from_playlist_option,
                                            const std::function<void()>& callback_close) {
    if (!widget) { return; }
    if (!db::track_by_id(ti.track_id)) { return; }
    bool is_loved = db::playlist_loved_tracks().has_track_id(ti.track_id);
    bool is_user_playlist = db::playlist_by_id(ti.playlist_id).value().get().type == db::PlaylistType::USER;

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
        player::enqueue(player::QueueTrack(ti),
                        player::get_playing_index().value_or(player::get_playing_queue().size()));

        notifications->push(
          tr::format("notification.appended_to_queue", db::track_by_id(ti.track_id)->get().pretty_name()));
        if (callback_close) { callback_close(); }
      },
      "play_next");

    buttons.emplace_back(
      tr::get("popover.track.append_to_queue"),
      [ti, callback_close]() -> void {
        player::enqueue(player::QueueTrack(ti), player::get_playing_queue().size());

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

    bool is_album = db::playlist_by_id(ti.playlist_id)->get().type == db::PlaylistType::ALBUM;
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
              panel_tracklist->scroll_to_track(album_id, track_id, immediate);
              panel_playlists->scroll_to_playlist(album_id, immediate);
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
          panel_tracklist->recreate(active_collection_id);
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

  void show_popover_tracklist_tracks_actions(size_t item_index, const std::function<void()>& callback_close) {
    Widget* widget =
      std::visit([](auto& item) -> Widget* { return item.widget; }, panel_tracklist->get_item(item_index));
    if (!widget) { return; }

    std::vector<db::track_info> tracks;
    tracks.reserve(panel_tracklist->selection().size());
    for (auto i : panel_tracklist->selection().get()) {
      ensure(panel_tracklist->get_items().size() > i);
      if (auto* item_track = std::get_if<PanelTracklist::ItemTrack>(&panel_tracklist->get_items()[i])) {
        tracks.emplace_back(item_track->track_info);
      }
    }

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
                            db::playlist_by_id(common_playlist_id.value()).value().get().type == db::PlaylistType::USER;
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
            panel_tracklist->recreate(active_collection_id);
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

  void show_popover_collection_actions(db::collection_id_t collection_id, Widget* widget) {
    if (collection_id == 0) { return; }
    vec2i at = widget->get_position(Anchor::CENTER);
    decltype(popover_descriptor::buttons) buttons;
    buttons.emplace_back(
      tr::get("dialog.action.rename"), [collection_id]() { show_popup_rename_collection(collection_id); },
      "rename_collection");

    buttons.emplace_back(
      tr::get("dialog.action.set_sources"), [collection_id]() { show_popup_set_sources(collection_id); },
      "set_sources");

    buttons.emplace_back(
      tr::get("dialog.action.rescan"), [collection_id]() { zincbox::scanner::scan_collection(collection_id); },
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

  void show_popover_queue_item_actions(size_t item_index) {
    ensure(panel_queue->get_items().size() > item_index);
    auto& item = panel_queue->get_items()[item_index];

    size_t queue_index = 0;
    Widget* widget = nullptr;

    std::visit(
      [&](auto& i) {
        using T = std::decay_t<decltype(i)>;
        if constexpr (std::is_same_v<T, PanelTracklist::ItemQueueTrack> ||
                      std::is_same_v<T, PanelTracklist::ItemQueuePlaylist> ||
                      std::is_same_v<T, PanelTracklist::ItemQueueCollection>) {
          queue_index = i.queue_index;
          widget = i.widget;
        }
      },
      item);

    ensure(widget);
    ensure(player::get_playing_queue().size() > queue_index);

    auto q_elem = player::get_playing_queue()[queue_index];
    decltype(popover_descriptor::buttons) buttons;

    buttons.emplace_back(
      tr::get("popover.queue.remove"),
      [queue_index]() {
        player::remove_from_queue(queue_index);
        panel_queue->on_queue_changed();
      },
      "remove_from_queue");

    if (q_elem.is_track()) {
      auto track_info = q_elem.track()->track_info;
      size_t track_id = track_info.track_id;
      size_t playlist_id = track_info.playlist_id;
      size_t collection_id = track_info.collection_id;
      bool is_loved = db::playlist_loved_tracks().has_track_id(track_id);

      bool is_album = db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::ALBUM;

      buttons.emplace_back(
        is_album ? tr::get("popover.track.show_in_album") : tr::get("popover.track.show_in_playlist"),
        [collection_id, playlist_id, track_id]() {
          show_collection(collection_id);
          panel_tracklist->scroll_to_track(playlist_id, track_id);
        },
        is_album ? "show_in_album" : "show_in_playlist");

      if (!is_album) {
        buttons.emplace_back(
          tr::get("popover.track.show_in_album"),
          [track_id]() -> void {
            auto& track = db::track_by_id(track_id)->get();
            auto album_id = track.originating_album_id;
            if (album_id != db::INVALID_ID) {
              auto collection_id_ = db::collection_of_playlist(album_id);
              if (collection_id_.has_value()) {
                show_collection(collection_id_.value());
                panel_tracklist->scroll_to_track(album_id, track_id);
              }
            }
          },
          "show_in_album");
      }

      if (!is_loved) {
        buttons.emplace_back(
          tr::get("popover.track.love"),
          [track_id, queue_index]() -> void {
            love_track(track_id);
            panel_queue->on_queue_changed_at(queue_index);
          },
          "love_track");
      } else {
        buttons.emplace_back(
          tr::get("popover.track.unlove"),
          [track_id, queue_index]() -> void {
            unlove_track(track_id);
            panel_queue->on_queue_changed_at(queue_index);
          },
          "unlove_track");
      }

      buttons.emplace_back(
        tr::get("popover.track.add_to_playlist"), [track_id]() -> void { show_add_to_playlist_popup(track_id); },
        "add_to_playlist");

    } else if (q_elem.is_playlist()) {
      auto qp = q_elem.playlist();
      size_t collection_id = qp->get_collection_id();
      auto playlist_id_opt = qp->get_playlist_id();
      bool is_album = db::playlist_by_id(*playlist_id_opt)->get().type == db::PlaylistType::ALBUM;
      if (playlist_id_opt) {
        buttons.emplace_back((is_album ? tr::get("popover.album.show") : tr::get("popover.playlist.show")),
                             [collection_id, playlist_id = playlist_id_opt.value()]() {
                               show_collection(collection_id);
                               panel_tracklist->scroll_to_playlist(playlist_id);
                             },
                             is_album ? "show_in_album" : "show_in_playlist");
      }

    } else if (q_elem.is_collection()) {
      size_t collection_id = q_elem.collection()->get_collection_id();

      buttons.emplace_back(
        tr::get("popover.collection.show"), [collection_id]() { show_collection(collection_id); }, "show_in_playlist");
    }

    vec2i at = widget->get_position(Anchor::CENTER);
    at.x = Input::get_mouse_x();
    popover_descriptor d{
      .id = "queue_item_actions",
      .title = "",
      .at = at,
      .distance = 4,
      .buttons = buttons,
    };
    popup_controller->create_popover(d);
  }

  void show_popover_queue_items_actions(size_t item_index, const std::function<void()>& callback_close) {
    auto items_span = panel_queue->get_items();
    ensure(items_span.size() > item_index);

    auto& clicked_item = items_span[item_index];
    zincgui::Widget* widget = nullptr;
    std::visit(
      [&](auto& i) {
        using T = std::decay_t<decltype(i)>;
        if constexpr (T::is_selectable) { widget = i.widget; }
      },
      clicked_item);
    ensure(widget);

    std::vector<size_t> queue_indices;
    std::vector<db::track_id_t> track_ids;
    bool only_tracks_selected = true;

    const auto& selection = panel_queue->selection().get();
    queue_indices.reserve(selection.size());

    for (size_t selection_i : selection) {
      if (selection_i >= items_span.size()) { continue; }

      auto& item = items_span[selection_i];
      size_t q_idx = 0;
      bool valid = false;

      std::visit(
        [&](auto& v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PanelTracklist::ItemQueueTrack> ||
                        std::is_same_v<T, PanelTracklist::ItemQueuePlaylist> ||
                        std::is_same_v<T, PanelTracklist::ItemQueueCollection>) {
            q_idx = v.queue_index;
            valid = true;
          }
        },
        item);

      if (!valid) { continue; }
      queue_indices.push_back(q_idx);

      if (q_idx < player::get_playing_queue().size()) {
        auto q_elem = player::get_playing_queue()[q_idx];

        if (q_elem.is_track()) {
          track_ids.push_back(q_elem.track()->track_info.track_id);
        } else {
          only_tracks_selected = false;
        }
      }
    }

    std::sort(queue_indices.begin(), queue_indices.end());
    queue_indices.erase(std::unique(queue_indices.begin(), queue_indices.end()), queue_indices.end());

    bool all_tracks_loved = true;
    bool all_tracks_not_loved = true;

    if (only_tracks_selected && !track_ids.empty()) {
      std::sort(track_ids.begin(), track_ids.end());
      track_ids.erase(std::unique(track_ids.begin(), track_ids.end()), track_ids.end());

      for (auto tid : track_ids) {
        bool is_loved = db::playlist_loved_tracks().has_track_id(tid);
        if (!is_loved) { all_tracks_loved = false; }
        if (is_loved) { all_tracks_not_loved = false; }
      }
    }

    decltype(popover_descriptor::buttons) buttons;

    buttons.emplace_back(
      tr::get("popover.track.play_next_plural"),
      [queue_indices, callback_close]() -> void {
        auto playing_index = player::get_playing_index();
        if (playing_index.has_value()) {
          player::move_queue_tracks(queue_indices, playing_index.value() + 1);
        } else {
          player::move_queue_tracks(queue_indices, 0);
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

    if (only_tracks_selected && !track_ids.empty()) {
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
    }

    vec2i at = widget->get_position(Anchor::CENTER);
    at.x = Input::get_mouse_x();
    popover_descriptor d{
      .id = "queue_items_actions",
      .title = tr::format("popover.track.title_plural", queue_indices.size()),
      .at = at,
      .distance = 4,
      .buttons = buttons,
    };
    popup_controller->create_popover(d);
  }

  void show_popover_playlist_actions(db::playlist_id_t playlist_id, Widget* widget, bool play_actions,
                                     const std::function<void()>& callback_close) {
    decltype(popover_descriptor::buttons) buttons;

    bool is_playlist = db::playlist_by_id(playlist_id)->get().type != db::PlaylistType::ALBUM;
    bool is_album = db::playlist_by_id(playlist_id)->get().type == db::PlaylistType::ALBUM;

    if (play_actions) {
      buttons.emplace_back((tr::get("popover.playlist.play")),
                           [callback_close, playlist_id]() -> void {
                             if (callback_close) { callback_close(); }
                             if (panel_playlists->get_collection_id().has_value()) {
                               player::play_playlist(*(panel_playlists->get_collection_id()), playlist_id, true);
                             }
                           },
                           "play_track");

      buttons.emplace_back((tr::get("popover.playlist.play_next")),
                           [callback_close, playlist_id]() -> void {
                             if (callback_close) { callback_close(); }
                             if (panel_playlists->get_collection_id().has_value()) {
                               if (player::play_playlist(*(panel_playlists->get_collection_id()), playlist_id, false)) {
                                 notifications->push(tr::format("notification.appended_to_queue",
                                                                db::playlist_by_id(playlist_id)->get().name));
                               }
                             }
                           },
                           "play_next");
    }

    if (is_playlist && playlist_id != 0) {
      buttons.emplace_back((tr::get("dialog.action.rename")),
                           [callback_close, playlist_id]() -> void {
                             if (callback_close) { callback_close(); }
                             show_popup_rename_playlist(playlist_id);
                           },
                           "rename_playlist");
    }

    if (is_playlist) {

      buttons.emplace_back(
        (tr::get("dialog.action.save_as_json")),
        [callback_close, playlist_id]() -> void {
          if (callback_close) { callback_close(); }
          if (file_dialog_type != FileDialogOpen::NONE) { return; }

          auto& playlist = db::playlist_by_id(playlist_id)->get();
          std::string json;
          auto ec = glz::write<glz::opts{.prettify = true}>(PlaylistSerialized(playlist), json);
          if (ec) {
            notifications->push_error(tr::format("notification.save_playlist_to_json_error", ec.custom_error_message));
            return;
          }

          file_dialog_type = FileDialogOpen::SAVE_JSON;
          file_dialog_data = std::move(json);
          std::string json_filter_label = tr::get("dialog.filter.json_files");
          auto file_name_utf8 = playlist.name + ".json";

          file_dialog_future_path = zincbox::ui::file_dialog::save_json_dialog(file_name_utf8, json_filter_label);
        },
        "save_playlist_as_json");

      buttons.emplace_back((tr::get("dialog.action.pick_image_file")),
                           [callback_close, playlist_id]() -> void {
                             if (callback_close) { callback_close(); }
                             if (file_dialog_type != FileDialogOpen::NONE) { return; }

                             file_dialog_type = FileDialogOpen::PICK_PLAYLIST_IMAGE;
                             file_dialog_data = playlist_id;
                             std::string image_filter_label = tr::get("dialog.filter.image_files");

                             file_dialog_future_path = zincbox::ui::file_dialog::pick_image_dialog(image_filter_label);
                           },
                           "pick_playlist_cover");

      buttons.emplace_back((tr::get("dialog.action.reset_image")),
                           [callback_close, playlist_id]() -> void { notifications->push_error("Unimplemented"); },
                           "reset_playlist_cover");
    }

    if (is_album) {
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

    if (is_playlist && playlist_id != 0) {
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

  void show_popover_playlist_sort_options(db::playlist_id_t playlist_id, Widget* widget) {
    decltype(popover_descriptor::buttons) buttons;

    buttons.emplace_back(
      tr::get("sort.artist.name.asc"),
      [playlist_id]() -> void {
        db::sort_playlist_by_artist_asc(playlist_id);
        panel_playlists->recreate();
        panel_tracklist->recreate(active_collection_id);
      },
      "artist_asc");

    buttons.emplace_back(
      tr::get("sort.artist.name.desc"),
      [playlist_id]() -> void {
        db::sort_playlist_by_artist_desc(playlist_id);
        panel_playlists->recreate();
        panel_tracklist->recreate(active_collection_id);
      },
      "artist_desc");

    buttons.emplace_back(
      tr::get("sort.title.name.asc"),
      [playlist_id]() -> void {
        db::sort_playlist_by_name_asc(playlist_id);
        panel_playlists->recreate();
        panel_tracklist->recreate(active_collection_id);
      },
      "title_asc");

    buttons.emplace_back(
      tr::get("sort.title.name.desc"),
      [playlist_id]() -> void {
        db::sort_playlist_by_name_desc(playlist_id);
        panel_playlists->recreate();
        panel_tracklist->recreate(active_collection_id);
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

  void show_popover_create_playlist(Widget* w) {
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

  void show_popover_queue_actions(Widget* w) {
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
            0, db::Playlist{popup->text_input->label.get_text(), {""}, db::PlaylistType::USER});
          for (const auto& play : player::get_flattened_queue().tracks) {
            db::add_track_id_to_playlist(playlist_id, play.track_id);
          }
          if (active_collection_id == 0) {
            panel_playlists->props.collection_id = 0;
            panel_playlists->recreate();
            panel_tracklist->recreate(active_collection_id);
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

  void show_popup_new_playlist(const std::function<void(std::optional<db::playlist_id_t>)>& callback_close) {
    auto* popup = popup_controller->show_popup<PopupInput>();
    popup->set_size(300, 200);
    popup->title->set_text(tr::get("popup.playlist.add.title"));
    popup->btn_ok->get_label().set_text(tr::get("dialog.action.add"));
    popup->text_input->set_focused(true);

    popup->on_ok_pressed = [popup, callback_close]() {
      auto playlist_id = db::add_playlist_to_collection(
        0, db::Playlist{popup->text_input->label.get_text(), {""}, db::PlaylistType::USER});
      if (active_collection_id == 0) {
        panel_playlists->props.collection_id = 0;
        panel_playlists->recreate();
        panel_tracklist->recreate(active_collection_id);
      }
      if (callback_close) { callback_close(playlist_id); }
    };

    popup->on_cancel_pressed = [callback_close]() {
      if (callback_close) { callback_close(std::nullopt); }
    };
  }

  void show_popup_new_smart_playlist() { popup_controller->show_popup<PopupCreateSmartPlaylist>(); }

  void show_dialog_new_playlist_from_json() {}

  void add_track_to_playlist(db::playlist_id_t playlist_id, db::track_id_t track_id) {
    bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
    if (db::add_track_id_to_playlist(playlist_id, track_id)) {
      auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
      if (loved_tracks) {
        notifications->push(tr::format("notification.loved_track", track_pretty_name));
        if (player::get_playing_track() && track_id == player::get_playing_track()->track_id) {
          panel_controls->update_love_state(true);
        }
      } else {
        auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
        notifications->push(tr::format("notification.added_track_to_playlist", track_pretty_name, playlist_name));
      }
    }
    if (active_collection_id.has_value()) {
      panel_tracklist->recreate(active_collection_id);
    } else {
      panel_queue->recreate();
    }
  }

  void add_tracks_to_playlist(db::playlist_id_t playlist_id, std::span<const db::track_id_t> track_ids) {
    bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
    i32 added_count = 0;
    for (db::track_id_t track_id : track_ids) {
      if (db::add_track_id_to_playlist(playlist_id, track_id)) {
        added_count += 1;
        if (loved_tracks && player::get_playing_track() && track_id == player::get_playing_track()->track_id) {
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
      if (active_collection_id.has_value()) { panel_tracklist->recreate(active_collection_id); }
    }
  }

  void remove_track_from_playlist(db::playlist_id_t playlist_id, db::track_id_t track_id) {
    bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
    if (db::remove_track_id_from_playlist(playlist_id, track_id)) {
      auto track_pretty_name = db::track_by_id(track_id)->get().pretty_name();
      if (loved_tracks) {
        notifications->push(tr::format("notification.unloved_track", track_pretty_name));
        if (player::get_playing_track() && track_id == player::get_playing_track()->track_id) {
          panel_controls->update_love_state(false);
        }
      } else {
        auto playlist_name = db::playlist_by_id(playlist_id)->get().name;
        notifications->push(tr::format("notification.removed_track_from_playlist", track_pretty_name, playlist_name));
      }
    }
    if (active_collection_id.has_value()) {
      panel_tracklist->recreate(active_collection_id);
    } else {
      panel_queue->recreate();
    }
  }

  void remove_tracks_from_playlist(db::playlist_id_t playlist_id, std::span<const db::track_id_t> track_ids) {
    bool loved_tracks = playlist_id == db::playlist_loved_tracks_id();
    i32 removed_count = 0;
    for (db::track_id_t track_id : track_ids) {
      if (db::remove_track_id_from_playlist(playlist_id, track_id)) {
        removed_count += 1;
        if (loved_tracks && player::get_playing_track() && track_id == player::get_playing_track()->track_id) {
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

      if (active_collection_id.has_value()) { panel_tracklist->recreate(active_collection_id); }
    }
  }

  void love_track(db::track_id_t track_id) { add_track_to_playlist(db::playlist_loved_tracks_id(), track_id); }

  void love_tracks(std::span<const db::track_id_t> track_ids) {
    add_tracks_to_playlist(db::playlist_loved_tracks_id(), track_ids);
  }

  void unlove_track(db::track_id_t track_id) { remove_track_from_playlist(db::playlist_loved_tracks_id(), track_id); }

  void unlove_tracks(std::span<const db::track_id_t> track_ids) {
    remove_tracks_from_playlist(db::playlist_loved_tracks_id(), track_ids);
  }

  void show_search_popup() {
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
        panel_tracklist->scroll_to_playlist(playlist_id);
        popup->close();
        popup->on_closed();
      }
    };

    popup->on_track_lmb = [popup](db::track_info ti, Widget*) {
      show_collection(ti.collection_id);
      panel_tracklist->scroll_to_track(ti.playlist_id, ti.track_id);
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

  void show_settings_popup() {
    auto* popup = popup_controller->show_popup<PopupSettings>();
    popup->on_save = [](Settings new_settings) -> void {
      if (zincbox::settings().must_reload(new_settings)) {
        auto* popup_close = popup_controller->show_popup<PopupConfirm>(tr::get("popup.reload_required.content"));
        popup_close->title->set_text(tr::get("popup.reload_required.title"));
        popup_close->set_width(popup_close->content->get_width() + 32);
        popup_close->btn_ok->get_label().set_text(tr::get("dialog.action.reload"));
        popup_close->btn_cancel->get_label().set_text(tr::get("dialog.action.cancel"));
        popup_close->on_ok_pressed = []() { zincbox::stop(); };
      }
      zincbox::settings() = new_settings;
    };
  }

  void show_about_popup() { popup_controller->show_popup<PopupAbout>(); }

  void quit() { zincbox::stop(); }

  void handle_drag_and_drop_clear();
  void handle_drag_and_drop_start();

  void handle_drag(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                   WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item,
                   bool hovered_queue_panel, Tab* hovered_tab, bool hovered_playlists_panel);
  void handle_drop(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                   WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item,
                   bool hovered_queue_panel, Tab* hovered_tab, bool hovered_playlists_panel);

  bool handle_drag_tracklist(WidgetTrack* hovered_track);
  bool handle_drop_tracklist(WidgetTrack* hovered_track);
  bool handle_drag_playlists(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                             bool hovered_playlists_panel);
  bool handle_drop_playlists(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                             bool hovered_playlists_panel);
  bool handle_drag_queue(WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item);
  bool handle_drop_queue(WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item);

  bool handle_drag_playlists_empty_space();
  bool handle_drop_playlists_empty_space();
  bool handle_drag_playlist_cover(db::playlist_id_t playlist_id);
  bool handle_drop_playlist_cover(db::playlist_id_t playlist_id);

  bool handle_drag_tab(Tab* hovered_tab);
  bool handle_drop_tab(Tab* hovered_tab);

  std::vector<db::track_id_t> get_selection_track_ids(const std::vector<PanelTracklist::Item>& items) {
    std::vector<db::track_id_t> track_ids;
    track_ids.reserve(items.size());
    for (const auto& item : items) {
      if (auto* t = std::get_if<PanelTracklist::ItemTrack>(&item)) {
        if (t->track_info.track_id != db::INVALID_ID) { track_ids.emplace_back(t->track_info.track_id); }
      } else if (auto* qt = std::get_if<PanelTracklist::ItemQueueTrack>(&item)) {
        if (qt->queue_index < player::get_playing_queue().size()) {
          auto q_elem = player::get_playing_queue()[qt->queue_index];
          if (q_elem.is_track() && q_elem.track()->track_info.track_id != db::INVALID_ID) {
            track_ids.emplace_back(q_elem.track()->track_info.track_id);
          }
        }
      }
    }
    return track_ids;
  }

  void handle_drag_and_drop_clear() {
    selection.items.clear();
    selection.items_sorted.clear();
    selection.common_playlist_id.reset();
    selection.start = vec2i{};
    selection.started = false;
    selection.drag_tab_id = -1;
    selection.drag_tab_timer = 0;
  }

  void handle_drag_and_drop_start() {
    bool lmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool lmb_pressed = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool lmb_just_released = Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool lmb = lmb_just_pressed || lmb_pressed || lmb_just_released;

    if (popup_controller->is_popup_open() || !lmb_just_pressed) { return; }

    WidgetAlbumCover* hovered_playlist_cover = nullptr;
    WidgetTrack* hovered_track = nullptr;
    WidgetQueuePlaylistCollection* hovered_queue_item = nullptr;
    WidgetPlaylistHeader* hovered_playlist_header = nullptr;
    Tab* hovered_tab = nullptr;

    if (lmb) {
      for (Widget* hovered_widget : root->get_hovered_widgets()) {
        if (hovered_playlist_cover = dynamic_cast<WidgetAlbumCover*>(hovered_widget); hovered_playlist_cover) { break; }
        if (hovered_track = dynamic_cast<WidgetTrack*>(hovered_widget); hovered_track) { break; }
        if (hovered_queue_item = dynamic_cast<WidgetQueuePlaylistCollection*>(hovered_widget); hovered_queue_item) {
          break;
        }
        if (hovered_playlist_header = dynamic_cast<WidgetPlaylistHeader*>(hovered_widget); hovered_playlist_header) {
          break;
        }
        if (hovered_tab = dynamic_cast<Tab*>(hovered_widget); hovered_tab) {
          if (selection.drag_tab_id != hovered_tab->id) {
            selection.drag_tab_id = hovered_tab->id;
            selection.drag_tab_timer = 0;
          }
          break;
        }
      }
    }

    handle_drag_and_drop_clear();

    if (panel_tracklist->get_is_drawn() && !panel_tracklist->selection().empty()) {
      bool hovering = hovered_track && hovered_track->is_selected();
      if (!hovering) { return; }
    }

    if (panel_queue->get_is_drawn() && !panel_queue->selection().empty()) {
      bool hovering = hovered_track && hovered_track->is_selected();
      hovering |= hovered_queue_item && hovered_queue_item->is_selected();
      if (!hovering) { return; }
    }

    if (panel_tracklist->get_is_drawn()) {
      if (panel_tracklist->selection().empty() && hovered_track && !Input::ctrl_pressed()) {
        panel_tracklist->insert_to_selection_index(hovered_track->item_index());
        ensure(hovered_track->track_info().track_id != db::INVALID_ID);
      }

      if (!panel_tracklist->selection().empty()) {
        selection.start = Input::get_mouse_pos();
        bool first = true;
        for (size_t i : panel_tracklist->selection().get()) {
          auto item = panel_tracklist->get_items()[i];
          if (auto* t = std::get_if<PanelTracklist::ItemTrack>(&item)) {
            if (first) {
              selection.common_playlist_id = t->track_info.playlist_id;
              first = false;
            } else if (selection.common_playlist_id != t->track_info.playlist_id) {
              selection.common_playlist_id = std::nullopt;
            }
            ensure(t->track_info.track_id != db::INVALID_ID);
          } else if (!std::holds_alternative<PanelTracklist::ItemPlaylistHeader>(item)) {
            out::debug_warn("start selection unknown item");
          }
          selection.items.emplace_back(item);
        }
        selection.source = SelectionSource::COLLECTION;
      }
    } else if (panel_queue->get_is_drawn()) {
      if (panel_queue->selection().empty() && !Input::ctrl_pressed()) {
        if (hovered_track) {
          panel_queue->insert_to_selection(hovered_track->item_index());
        } else if (hovered_queue_item) {
          panel_queue->insert_to_selection(hovered_queue_item->queue_index());
        }
      }

      if (!panel_queue->selection().empty()) {
        selection.items.clear();
        selection.start = Input::get_mouse_pos();
        bool first = true;
        for (size_t i : panel_queue->selection().get()) {
          auto item = panel_queue->get_items()[i];
          if (auto* qt = std::get_if<PanelTracklist::ItemQueueTrack>(&item)) {
            if (qt->queue_index < player::get_playing_queue().size()) {
              auto q_elem = player::get_playing_queue()[qt->queue_index];
              if (q_elem.is_track()) {
                if (first) {
                  selection.common_playlist_id = q_elem.track()->track_info.playlist_id;
                  first = false;
                } else if (selection.common_playlist_id != q_elem.track()->track_info.playlist_id) {
                  selection.common_playlist_id = std::nullopt;
                }
                ensure(q_elem.track()->track_info.track_id != db::INVALID_ID);
              }
            }
          }
          selection.items.emplace_back(item);
        }
        selection.source = SelectionSource::QUEUE;
      }
    }

    selection.items_sorted = selection.items;
    std::sort(selection.items_sorted.begin(), selection.items_sorted.end(),
              [](const PanelTracklist::Item& a, const PanelTracklist::Item& b) -> bool {
                auto list_index = [](const PanelTracklist::Item& item) -> size_t {
                  return std::visit([](const auto& val) { return val.item_index; }, item);
                };
                return list_index(a) < list_index(b);
              });
  }

  bool handle_drag_playlist_cover(db::playlist_id_t playlist_id) {
    auto playlist = db::playlist_by_id(playlist_id);
    if (!playlist.has_value()) { return false; }
    if (playlist->get().type != db::PlaylistType::USER) {
      tooltip_drag->set_text(tr::get("tooltip.drag.not_allowed"));
      return true;
    }
    size_t track_count = get_selection_track_ids(selection.items).size();
    if (track_count == 0) { return false; }

    if (track_count > 1) {
      tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist_plural", track_count, playlist->get().name));
    } else {
      tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist", playlist->get().name));
    }
    return true;
  }

  bool handle_drop_playlist_cover(db::playlist_id_t playlist_id) {
    auto playlist = db::playlist_by_id(playlist_id);
    if (!playlist.has_value() || playlist->get().type != db::PlaylistType::USER) { return false; }
    auto track_ids = get_selection_track_ids(selection.items);
    if (track_ids.empty()) { return false; }

    add_tracks_to_playlist(playlist_id, track_ids);
    return true;
  }

  bool handle_drag_playlists_empty_space() {
    if (get_selection_track_ids(selection.items).empty()) { return false; }
    tooltip_drag->set_text(tr::get("tooltip.drag.create_playlist"));
    return true;
  }

  bool handle_drop_playlists_empty_space() {
    auto track_ids = get_selection_track_ids(selection.items);
    if (track_ids.empty()) { return false; }

    show_popup_new_playlist([track_ids](std::optional<db::playlist_id_t> playlist_id) -> void {
      if (playlist_id.has_value()) { add_tracks_to_playlist(playlist_id.value(), track_ids); }
    });
    return true;
  }

  bool handle_drag_playlists(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                             bool hovered_playlists_panel) {
    if (hovered_playlist_cover) {
      if (hovered_playlist_cover->is_add_button()) { return handle_drag_playlists_empty_space(); }
      if (hovered_playlist_cover->playlist_id.has_value()) {
        return handle_drag_playlist_cover(hovered_playlist_cover->playlist_id.value());
      }
    }
    if (hovered_playlist_header) { return handle_drag_playlist_cover(hovered_playlist_header->playlist_id); }
    if (hovered_playlists_panel) { return handle_drag_playlists_empty_space(); }
    return false;
  }

  bool handle_drop_playlists(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                             bool hovered_playlists_panel) {
    if (hovered_playlist_cover) {
      if (hovered_playlist_cover->is_add_button()) { return handle_drop_playlists_empty_space(); }
      if (hovered_playlist_cover->playlist_id.has_value()) {
        return handle_drop_playlist_cover(hovered_playlist_cover->playlist_id.value());
      }
    }
    if (hovered_playlist_header) { return handle_drop_playlist_cover(hovered_playlist_header->playlist_id); }
    if (hovered_playlists_panel) { return handle_drop_playlists_empty_space(); }
    return false;
  }

  bool handle_drag_tracklist(WidgetTrack* hovered_track) {
    if (!hovered_track) { return false; }
    auto target_playlist_id = hovered_track->playlist_id();
    auto playlist = db::playlist_by_id(target_playlist_id);
    if (!playlist.has_value()) { return false; }
    bool above = hovered_track->get_position(Anchor::CENTER).y > Input::get_mouse_y();
    auto insert_cursor_pos = above ? PanelTracklist::InsertCursorPos::ABOVE : PanelTracklist::InsertCursorPos::BELOW;

    panel_tracklist->set_insert_cursor_index(hovered_track->item_index());
    panel_tracklist->set_insert_cursor_pos(insert_cursor_pos);

    bool move =
      (selection.source == SelectionSource::COLLECTION) && (selection.common_playlist_id == target_playlist_id);

    if (playlist->get().type != db::PlaylistType::USER) {
      tooltip_drag->set_text(tr::get("tooltip.drag.not_allowed"));
    } else if (move) {
      size_t move_count = 0;
      for (const auto& item : selection.items) {
        if (std::holds_alternative<PanelTracklist::ItemTrack>(item)) { move_count += 1; }
      }
      if (move_count == 0) { return false; }

      if (move_count > 1) {
        tooltip_drag->set_text(tr::format("tooltip.drag.reorder_plural", move_count));
      } else {
        tooltip_drag->set_text(tr::get("tooltip.drag.reorder"));
      }
    } else {
      auto playlist_name = playlist->get().name;
      size_t track_count = get_selection_track_ids(selection.items).size();
      if (track_count == 0) { return false; }

      if (track_count > 1) {
        tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist_plural", track_count, playlist_name));
      } else {
        tooltip_drag->set_text(tr::format("tooltip.drag.add_to_playlist", playlist_name));
      }
    }
    return true;
  }

  bool handle_drop_tracklist(WidgetTrack* hovered_track) {
    if (!hovered_track) { return false; }
    auto target_playlist_id = hovered_track->playlist_id();
    auto playlist = db::playlist_by_id(target_playlist_id);
    if (!playlist.has_value() || playlist->get().type != db::PlaylistType::USER) { return false; }
    ensure(db::collection_of_playlist(target_playlist_id) == 0);
    size_t i = hovered_track->track_info().index;
    bool above = hovered_track->get_position(Anchor::CENTER).y > Input::get_mouse_y();
    bool move =
      (selection.source == SelectionSource::COLLECTION) && (selection.common_playlist_id == target_playlist_id);
    if (!above) { i += 1; }

    std::vector<size_t> new_track_indices;

    if (move) {
      std::vector<size_t> track_indices;
      track_indices.reserve(selection.items_sorted.size());
      for (const auto& item : selection.items_sorted) {
        if (auto* t = std::get_if<PanelTracklist::ItemTrack>(&item)) {
          ensure(t->track_info.track_id != db::INVALID_ID);
          track_indices.emplace_back(t->track_info.index);
        }
      }
      if (track_indices.empty()) { return false; }
      new_track_indices = db::move_track_indices_in_playlist(target_playlist_id, track_indices, i);
    } else {
      auto track_ids = get_selection_track_ids(selection.items_sorted);
      if (track_ids.empty()) { return false; }
      new_track_indices = db::add_track_ids_to_playlist(target_playlist_id, i, track_ids);
    }

    if (new_track_indices.size() == 1) { new_track_indices.clear(); }

    if (active_collection_id.has_value()) {
      panel_tracklist->recreate(active_collection_id);
      panel_tracklist->clear_selection();
      for (size_t idx : new_track_indices) {
        auto track_id = playlist->get().get_track_ids()[idx];
        auto track_info =
          db::track_info{.collection_id = 0, .playlist_id = target_playlist_id, .track_id = track_id, .index = idx};
        panel_tracklist->insert_to_selection_track(track_info);
      }
    }

    return true;
  }

  bool handle_drag_queue(WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item) {
    if (hovered_track) {
      bool above = hovered_track->get_position(Anchor::CENTER).y > Input::get_mouse_y();
      panel_queue->set_insert_cursor_index(hovered_track->item_index());
      panel_queue->set_insert_cursor_pos(above ? PanelTracklist::InsertCursorPos::ABOVE
                                               : PanelTracklist::InsertCursorPos::BELOW);
    } else if (hovered_queue_item) {
      bool above = hovered_queue_item->get_position(Anchor::CENTER).y > Input::get_mouse_y();
      panel_queue->set_insert_cursor_index(hovered_queue_item->queue_index());
      panel_queue->set_insert_cursor_pos(above ? PanelTracklist::InsertCursorPos::ABOVE
                                               : PanelTracklist::InsertCursorPos::BELOW);
    } else {
      if (!player::get_playing_queue().empty()) {
        panel_queue->set_insert_cursor_index(player::get_playing_queue().size() - 1);
        panel_queue->set_insert_cursor_pos(PanelTracklist::InsertCursorPos::BELOW);
      }
    }

    bool move = (selection.source == SelectionSource::QUEUE);
    size_t valid_count = 0;

    if (move) {
      for (const auto& item : selection.items) {
        std::visit(
          [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, PanelTracklist::ItemQueueTrack> ||
                          std::is_same_v<T, PanelTracklist::ItemQueuePlaylist> ||
                          std::is_same_v<T, PanelTracklist::ItemQueueCollection>) {
              valid_count++;
            }
          },
          item);
      }
    } else {
      for (const auto& item : selection.items) {
        if (std::holds_alternative<PanelTracklist::ItemTrack>(item)) { valid_count++; }
      }
    }

    if (valid_count == 0) { return false; }

    if (move) {
      if (valid_count > 1) {
        tooltip_drag->set_text(tr::format("tooltip.drag.reorder_plural", valid_count));
      } else {
        tooltip_drag->set_text(tr::get("tooltip.drag.reorder"));
      }
    } else {
      if (valid_count > 1) {
        tooltip_drag->set_text(tr::format("tooltip.drag.add_to_queue_plural", valid_count));
      } else {
        tooltip_drag->set_text(tr::get("tooltip.drag.add_to_queue"));
      }
    }
    return true;
  }

  bool handle_drop_queue(WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item) {
    size_t target_index = 0;
    if (hovered_track) {
      target_index = hovered_track->item_index();
      bool above = hovered_track->get_position(Anchor::CENTER).y > Input::get_mouse_y();
      if (!above) { target_index += 1; }
    } else if (hovered_queue_item) {
      target_index = hovered_queue_item->queue_index();
      bool above = hovered_queue_item->get_position(Anchor::CENTER).y > Input::get_mouse_y();
      if (!above) { target_index += 1; }
    } else {
      target_index = player::get_playing_queue().size();
    }

    bool move = (selection.source == SelectionSource::QUEUE);
    std::vector<size_t> selection_indices_to_restore;

    if (move) {
      std::vector<size_t> indices;
      indices.reserve(selection.items_sorted.size());
      for (const auto& item : selection.items_sorted) {
        std::visit(
          [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, PanelTracklist::ItemQueueTrack> ||
                          std::is_same_v<T, PanelTracklist::ItemQueuePlaylist> ||
                          std::is_same_v<T, PanelTracklist::ItemQueueCollection>) {
              indices.emplace_back(v.queue_index);
            }
          },
          item);
      }
      if (indices.empty()) { return false; }
      selection_indices_to_restore = player::move_queue_tracks(indices, target_index);
    } else {
      std::vector<db::track_info> track_infos;
      track_infos.reserve(selection.items_sorted.size());
      for (const auto& item : selection.items_sorted) {
        if (auto* t = std::get_if<PanelTracklist::ItemTrack>(&item)) { track_infos.emplace_back(t->track_info); }
      }
      if (track_infos.empty()) { return false; }
      selection_indices_to_restore = player::add_to_queue(track_infos, target_index);
    }

    if (selection_indices_to_restore.size() == 1) { selection_indices_to_restore.clear(); }

    panel_queue->on_queue_changed();
    panel_queue->clear_selection();
    for (size_t i : selection_indices_to_restore) {
      panel_queue->insert_to_selection(i);
    }
    return true;
  }

  bool handle_drag_tab(Tab* hovered_tab) {
    if (!hovered_tab) { return false; }
    if (selection.drag_tab_timer < SELECTION_DRAG_TAB_TIMER) {
      selection.drag_tab_timer += 1;
    } else {
      panel_top->select(hovered_tab->id);
    }
    return true;
  }

  bool handle_drop_tab(Tab*) { return false; }

  void handle_drag(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                   WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item,
                   bool hovered_queue_panel, Tab* hovered_tab, bool hovered_playlists_panel) {
    tooltip_drag->set_is_drawn(true);
    tooltip_drag->set_pos(Input::get_mouse_pos() + vec2i{0, 10});
    panel_tracklist->set_is_dragged(true);
    panel_queue->set_is_dragged(true);

    if (handle_drag_playlists(hovered_playlist_cover, hovered_playlist_header, hovered_playlists_panel)) { return; }
    if (panel_tracklist->get_is_drawn() && hovered_track) {
      if (handle_drag_tracklist(hovered_track)) { return; }
    } else if (panel_queue->get_is_drawn() && (hovered_track || hovered_queue_item || hovered_queue_panel)) {
      if (handle_drag_queue(hovered_track, hovered_queue_item)) { return; }
    }
    if (hovered_tab && handle_drag_tab(hovered_tab)) { return; }

    if (selection.items.size() > 1) {
      tooltip_drag->set_text(tr::format("tooltip.drag.tracks", selection.items.size()));
    } else {
      tooltip_drag->set_text(tr::get("tooltip.drag.track"));
    }
  }

  void handle_drop(WidgetAlbumCover* hovered_playlist_cover, WidgetPlaylistHeader* hovered_playlist_header,
                   WidgetTrack* hovered_track, WidgetQueuePlaylistCollection* hovered_queue_item,
                   bool hovered_queue_panel, Tab* hovered_tab, bool hovered_playlists_panel) {
    bool handled = false;

    if (handle_drop_playlists(hovered_playlist_cover, hovered_playlist_header, hovered_playlists_panel)) {
      handled = true;
    } else if (panel_tracklist->get_is_drawn() && hovered_track) {
      handled = handle_drop_tracklist(hovered_track);
    } else if (panel_queue->get_is_drawn() && (hovered_track || hovered_queue_item || hovered_queue_panel)) {
      handled = handle_drop_queue(hovered_track, hovered_queue_item);
    } else if (hovered_tab) {
      handled = handle_drop_tab(hovered_tab);
    }

    if (handled) {
      selection.items_sorted.clear();
      panel_tracklist->set_is_dragged(false);
      panel_queue->set_is_dragged(false);
    }
  }

  void handle_drag_and_drop() {
    bool lmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool rmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_RIGHT);
    bool lmb_pressed = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool lmb_just_released = Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT);
    bool lmb = lmb_just_pressed || lmb_pressed || lmb_just_released;

    WidgetAlbumCover* hovered_playlist_cover = nullptr;
    WidgetTrack* hovered_track = nullptr;
    WidgetQueuePlaylistCollection* hovered_queue_item = nullptr;
    WidgetPlaylistHeader* hovered_playlist_header = nullptr;
    Tab* hovered_tab = nullptr;

    bool hovered_queue_panel = panel_queue->is_mouse_hovering();
    bool hovered_playlists_panel = panel_playlists->is_mouse_hovering();

    if (lmb) {
      for (Widget* hovered_widget : root->get_hovered_widgets()) {
        if (hovered_playlist_cover = dynamic_cast<WidgetAlbumCover*>(hovered_widget); hovered_playlist_cover) { break; }
        if (hovered_track = dynamic_cast<WidgetTrack*>(hovered_widget); hovered_track) { break; }
        if (hovered_queue_item = dynamic_cast<WidgetQueuePlaylistCollection*>(hovered_widget); hovered_queue_item) {
          break;
        }
        if (hovered_playlist_header = dynamic_cast<WidgetPlaylistHeader*>(hovered_widget); hovered_playlist_header) {
          break;
        }
        if (hovered_tab = dynamic_cast<Tab*>(hovered_widget); hovered_tab) { break; }
      }
    }

    if (lmb_just_pressed) { handle_drag_and_drop_start(); }

    if (lmb_pressed && !selection.items.empty() && !selection.started) {
      vec2i diff = selection.start - Input::get_mouse_pos();
      if (std::abs(diff.x) > 4 || std::abs(diff.y) > 4) { selection.started = true; }
    }

    bool valid_selection = !selection.items.empty() && selection.started;

    panel_tracklist->set_insert_cursor_index(std::nullopt);
    panel_queue->set_insert_cursor_index(std::nullopt);

    if (valid_selection) {
      if (lmb_pressed) {
        handle_drag(hovered_playlist_cover, hovered_playlist_header, hovered_track, hovered_queue_item,
                    hovered_queue_panel, hovered_tab, hovered_playlists_panel);
      } else if (lmb_just_released) {
        handle_drop(hovered_playlist_cover, hovered_playlist_header, hovered_track, hovered_queue_item,
                    hovered_queue_panel, hovered_tab, hovered_playlists_panel);
      }
    } else {
      tooltip_drag->set_is_drawn(false);
    }

    if (lmb_just_released || rmb_just_pressed || Input::key_just_pressed(Input::Key::KEY_ESCAPE) ||
        popup_controller->is_popup_open()) {
      handle_drag_and_drop_clear();
      panel_tracklist->set_is_dragged(false);
      panel_queue->set_is_dragged(false);
    }
  }
} // namespace
