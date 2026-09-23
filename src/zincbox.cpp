#include "zincbox.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/serialized_state.hpp"
#include "common/signal.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/mpris.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "core/settings.hpp"
#include "opengl_includes.hpp"
#include "theme_config.hpp"
#include "ui/interface.hpp"
#include "ui/sdl3_window.hpp"
#include "ui/theme.hpp"
#include "ui/tray.hpp"
#include "ui/zincgui/input.hpp"

static std::atomic<bool> s_running = false;
static Settings s_settings;
static std::unique_ptr<zincbox::SDL3Window> s_window = nullptr;
static AppSerialized s_loaded_state;

static void update_mini_player_state();
static void update_window_title();
static void check_opengl_errors();
static void update_mpris();

float zincbox::ui_scale() { return s_settings.interface.scale * 0.01f; }

void zincbox::init(u64 flags) {
  out::debug_info("db::deserialize start");
  if (std::filesystem::exists(io::get_db_path())) {
    auto s = std::ifstream{io::get_db_path(), std::ifstream::binary};
    db::deserialize(s);
  } else {
    db::create_empty_db();
  }
  out::debug_info("db::deserialize end");

  player::init();

  if (flags & zincbox::MPRIS) { mpris::init(); }

  if (flags & zincbox::TRAY) { tray::init(); }

  if (flags & zincbox::WINDOW) {
    s_window = std::make_unique<SDL3Window>();
    if (s_window->has_error()) {
      out::critical("failed to open window: {}", s_window->get_error());
      std::exit(1);
    }

    s_window->min_size(480, 320);
    s_window->max_size(7680, 4320);
    s_window->vsync(false);

    zincbox::ui::init();
  }
}

void zincbox::run() {
  s_window->make_current();
  if (s_running) { return; }
  s_running = true;
  while (s_running) {
    using namespace std::chrono;
    auto t1 = high_resolution_clock::now();
    s_window->poll_events();
    if (s_window->should_close()) { s_running = false; }
    glViewport(0, 0, s_window->width(), s_window->height());
    update_mini_player_state();
    update_window_title();
    update_mpris();
    zincgui::Input::update();
    player::update();
    tray::update();
    zincbox::ui::update(s_window->size());
    zincgui::Input::clear();
    check_opengl_errors();
    window()->swap_buffers();

    auto t2 = high_resolution_clock::now();
    long delta_us = duration_cast<microseconds>(t2 - t1).count();
    long sleep_us = std::max(1000.0, 16666.0 - delta_us);
    if (!window()->vsync()) { std::this_thread::sleep_for(microseconds(sleep_us)); }
  }
}

void zincbox::stop() { s_running = false; }

void zincbox::deinit() {
  tray::deinit();
  zincbox::ui::deinit();
  player::deinit();
  mpris::deinit();
  s_window = nullptr;
}

void zincbox::load_state_from_json() {
  auto ec = glz::read_file_json(s_loaded_state, path_to_utf8(io::get_cfg_path()).c_str(), std::string{});
  if (ec) {
    out::error("failed to read zincbox.json: {}", ec.custom_error_message);
    return;
  }

  s_settings = s_loaded_state.settings;
  s_settings.clamp_values();
}

void zincbox::apply_loaded_state() {
  player::set_repeat_mode(s_loaded_state.player.repeat_mode);
  player::set_shuffle_mode(s_loaded_state.player.shuffle_mode);
  player::set_volume(s_loaded_state.player.volume);
  player::seek_ms(s_loaded_state.player.timestamp);

  for (auto& track_serialized : s_loaded_state.player.queue) {
    auto track = db::find_track(track_serialized.artist, track_serialized.title, track_serialized.collection,
                                track_serialized.playlist, track_serialized.path);
    if (!track.has_value()) {
      if (s_loaded_state.player.queue_index.has_value()) {
        s_loaded_state.player.queue_index = (s_loaded_state.player.queue_index > 0 &&
                                             s_loaded_state.player.queue_index < player::get_playing_queue().size())
                                              ? std::make_optional(s_loaded_state.player.queue_index.value() - 1)
                                              : std::nullopt;
      }
      continue;
    }
    player::enqueue(track.value(), player::get_playing_queue().size());
  }

  player::set_playing_index(s_loaded_state.player.queue_index);
  player::seek_ms(s_loaded_state.player.timestamp);
  player::pause();
  player::signal_on_queue_changed.emit(false);
  player::signal_on_track_changed.emit();

  zincbox::ui::set_mini_player(s_loaded_state.interface.mini_player);

  zincbox::ui::set_playlists_scroll_offset(s_loaded_state.interface.playlists_scroll_offset);
  zincbox::ui::set_selected_tab(s_loaded_state.interface.selected_tab);
  zincbox::ui::set_tabs_order(s_loaded_state.interface.tabs_order);
  zincbox::ui::set_tracks_scroll_offset(s_loaded_state.interface.tracks_scroll_offset);
  if (s_window) {
    s_window->resize(s_loaded_state.interface.window_width, s_loaded_state.interface.window_height);
    if (s_loaded_state.interface.window_maximized) { s_window->maximize(); }
    s_window->set_decoration(!theme::config().custom_window_decoration.enabled);
  }
}

void zincbox::save_state_to_json() {
  std::vector<QueueTrackSerialized> queue;
  for (const db::track_info& ti : player::get_playing_queue()) {
    auto track_ = db::track_by_id(ti.track_id);
    if (!track_) { continue; }
    queue.emplace_back(QueueTrackSerialized(ti));
  }

  AppSerialized state;

  state.player = {
    .repeat_mode = player::get_repeat_mode(),
    .shuffle_mode = player::get_shuffle_mode(),
    .volume = player::get_volume(),
    .timestamp = player::get_current_time_ms(),
    .queue_index = player::get_playing_index().value_or(-1),
    .queue = queue,
  };

  state.settings = s_settings;

  state.interface = {
    .mini_player = zincbox::ui::get_mini_player(),
    .playlists_scroll_offset = zincbox::ui::get_playlists_scroll_offset(),
    .selected_tab = zincbox::ui::get_selected_tab(),
    .tabs_order = zincbox::ui::get_tabs_order(),
    .tracks_scroll_offset = zincbox::ui::get_tracks_scroll_offset(),
    .window_width = s_window ? s_window->width() : 0,
    .window_height = s_window ? s_window->height() : 0,
    .window_maximized = s_window ? s_window->is_maximized() : false,
  };

  auto ec =
    glz::write_file_json<glz::opts{.prettify = true}>(state, path_to_utf8(io::get_cfg_path()).c_str(), std::string{});
  if (ec) { out::error("failed to write zincbox.json: {}", ec.custom_error_message); }
}

void zincbox::save_db_to_file() {
  ScopeTimer st{"db::serialize"};
  auto s = std::ofstream{io::get_db_path(), std::ifstream::binary};
  db::serialize(s);
  s.flush();
}

zincbox::SDL3Window* zincbox::window() { return s_window.get(); }
Settings& zincbox::settings() { return s_settings; }

static void update_mini_player_state() {
  static std::optional<bool> is_mini_player = std::nullopt;
  if (is_mini_player != zincbox::ui::get_mini_player()) {

    is_mini_player = zincbox::ui::get_mini_player();

    if (is_mini_player.value()) {
      s_loaded_state.interface.window_height = s_window->height();

      int fixed_h = theme::config().panel_controls.height + theme::config().custom_window_decoration.border_size;
      s_window->min_size(480, fixed_h);
      s_window->max_size(7680, fixed_h);
      s_window->resize(s_window->width(), fixed_h);
    } else {
      s_window->min_size(480, 320);
      s_window->max_size(7680, 4320);
      s_window->height(s_loaded_state.interface.window_height);
    }
  }
}

static void update_window_title() {
  static std::optional<db::track_info> prev_playing;
  auto playing = player::get_playing();
  if (playing == prev_playing) { return; }
  prev_playing = playing;

  std::string window_title;
  if (playing.has_value()) {
    auto& track = db::track_by_id(playing->track_id)->get();
    window_title = "zincbox (" + track.pretty_name() + ")";
  } else {
    window_title = "zincbox";
  }

  s_window->title(window_title.c_str());
}

static void update_mpris() {
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

    case mpris::CommandType::SET: {
      // Absolute seek
      player::seek_ms(static_cast<i32>(cmd->value));
      break;
    }

    case mpris::CommandType::LOOP:
      switch (static_cast<mpris::LoopStatus>(cmd->value)) {
      case mpris::LoopStatus::NONE: player::set_repeat_mode(player::RepeatMode::OFF); break;
      case mpris::LoopStatus::TRACK: player::set_repeat_mode(player::RepeatMode::TRACK); break;
      case mpris::LoopStatus::PLAYLIST: player::set_repeat_mode(player::RepeatMode::ALBUM); break;
      }
      break;

    case mpris::CommandType::SHUFFLE:
      player::set_shuffle_mode(static_cast<bool>(cmd->value) ? player::ShuffleMode::ON : player::ShuffleMode::OFF);
      break;
    }
  }

  static i32 t = 0;
  if (t++ >= 120) {
    t = 0;
    mpris::notify_seeked(player::get_current_time_ms());
  }
}

static const char* get_opengl_error_string(GLenum err) {
  switch (err) {
  case GL_NO_ERROR: return "No error";
  case GL_INVALID_ENUM: return "Invalid enum";
  case GL_INVALID_VALUE: return "Invalid value";
  case GL_INVALID_OPERATION: return "Invalid operation";
  case GL_STACK_OVERFLOW: return "Stack overflow";
  case GL_STACK_UNDERFLOW: return "Stack underflow";
  case GL_OUT_OF_MEMORY: return "Out of memory";
  case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
  default: return "Unknown error";
  }
}

static void check_opengl_errors() {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    std::stringstream error_hex;
    error_hex << std::hex << error << ": " << get_opengl_error_string(error);
    out::debug_error("GL error 0x{}", error_hex.str());
  }
}
