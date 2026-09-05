#include "tray.hpp"
#include <csignal>
#include <cstring>
#include <SDL3/SDL.h>
#include <stb_image.h>
#include "common/logger.hpp"
#include "common/utf.hpp"
#include "lib/SDL/include/SDL3/SDL_video.h"
#include "player.hpp"
#include "tr.hpp"
#include "ui/theme.hpp"

namespace tray {
  namespace {
    enum class TrayIconState : u8 { STOPPED, PLAYING, PAUSED, NONE };

    SDL_Tray* system_tray = nullptr;
    SDL_TrayMenu* main_menu = nullptr;
    SDL_TrayEntry* entry_play_pause = nullptr;
    SDL_TrayEntry* entry_stop = nullptr;
    SDL_TrayEntry* entry_prev = nullptr;
    SDL_TrayEntry* entry_next = nullptr;
    SDL_TrayEntry* entry_shuffle = nullptr;
    SDL_TrayMenu* repeat_menu = nullptr;
    SDL_TrayEntry* entry_repeat_off = nullptr;
    SDL_TrayEntry* entry_repeat_track = nullptr;
    SDL_TrayEntry* entry_repeat_album = nullptr;
    SDL_TrayEntry* entry_show = nullptr;
    SDL_TrayEntry* entry_quit = nullptr;

    SDL_Surface* icon_stopped = nullptr;
    SDL_Surface* icon_playing = nullptr;
    SDL_Surface* icon_paused = nullptr;
    TrayIconState current_icon_state = TrayIconState::NONE;

    SDL_Window* window = NULL;

    std::string tr_utf8(const std::string& key) { return utf32_to_utf8(tr::get(key)); }

    SDL_Surface* load_icon_surface(const std::string& path) {
      auto raw = theme::get_raw_resource(path);
      if (raw.empty()) { return nullptr; }
      i32 w = 0, h = 0, ch = 0;
      stbi_uc* pixels = stbi_load_from_memory(raw.data(), static_cast<int>(raw.size()), &w, &h, &ch, 4);
      if (!pixels) { return nullptr; }
      SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
      if (surface) { std::memcpy(surface->pixels, pixels, (size_t)w * h * 4); }
      stbi_image_free(pixels);
      return surface;
    }

    void cb_play_pause(void*, SDL_TrayEntry*) {
      if (player::is_playing()) {
        player::pause();
      } else {
        player::resume();
      }
    }
    void cb_stop(void*, SDL_TrayEntry*) { player::stop(); }
    void cb_prev(void*, SDL_TrayEntry*) { player::prev_track(); }
    void cb_next(void*, SDL_TrayEntry*) { player::next_track(); }
    void cb_shuffle(void*, SDL_TrayEntry*) {
      auto mode = player::get_shuffle_mode();
      player::set_shuffle_mode(mode == player::ShuffleMode::ON ? player::ShuffleMode::OFF : player::ShuffleMode::ON);
    }
    void cb_repeat_off(void*, SDL_TrayEntry*) { player::set_repeat_mode(player::RepeatMode::OFF); }
    void cb_repeat_track(void*, SDL_TrayEntry*) { player::set_repeat_mode(player::RepeatMode::TRACK); }
    void cb_repeat_album(void*, SDL_TrayEntry*) { player::set_repeat_mode(player::RepeatMode::ALBUM); }
    void cb_show(void*, SDL_TrayEntry*) {
      if (window) { SDL_RestoreWindow(window); }
    }
    void cb_quit(void*, SDL_TrayEntry*) { std::raise(SIGINT); }
  } // namespace

  void init(SDL_Window* window_) {
    window = window_;

    icon_stopped = load_icon_surface("tray/stopped.png");
    icon_playing = load_icon_surface("tray/playing.png");
    icon_paused = load_icon_surface("tray/paused.png");

    system_tray = SDL_CreateTray(icon_stopped, "zincbox");
    if (!system_tray) {
      out::debug_error("failed to create tray icon");
      return;
    }
    current_icon_state = TrayIconState::STOPPED;

    main_menu = SDL_CreateTrayMenu(system_tray);
    entry_show = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.show").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_show, cb_show, nullptr);
    SDL_InsertTrayEntryAt(main_menu, -1, nullptr, 0);
    entry_play_pause = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.play").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_play_pause, cb_play_pause, nullptr);
    entry_stop = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.stop").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_stop, cb_stop, nullptr);
    entry_prev = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.prev").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_prev, cb_prev, nullptr);
    entry_next = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.next").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_next, cb_next, nullptr);
    SDL_InsertTrayEntryAt(main_menu, -1, nullptr, 0);
    entry_shuffle = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.shuffle").c_str(), SDL_TRAYENTRY_CHECKBOX);
    SDL_SetTrayEntryCallback(entry_shuffle, cb_shuffle, nullptr);
    SDL_TrayEntry* entry_repeat_parent =
      SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.repeat").c_str(), SDL_TRAYENTRY_SUBMENU);
    repeat_menu = SDL_CreateTraySubmenu(entry_repeat_parent);
    entry_repeat_off =
      SDL_InsertTrayEntryAt(repeat_menu, -1, tr_utf8("tray.repeat_off").c_str(), SDL_TRAYENTRY_CHECKBOX);
    SDL_SetTrayEntryCallback(entry_repeat_off, cb_repeat_off, nullptr);
    entry_repeat_track =
      SDL_InsertTrayEntryAt(repeat_menu, -1, tr_utf8("tray.repeat_track").c_str(), SDL_TRAYENTRY_CHECKBOX);
    SDL_SetTrayEntryCallback(entry_repeat_track, cb_repeat_track, nullptr);
    entry_repeat_album =
      SDL_InsertTrayEntryAt(repeat_menu, -1, tr_utf8("tray.repeat_album").c_str(), SDL_TRAYENTRY_CHECKBOX);
    SDL_SetTrayEntryCallback(entry_repeat_album, cb_repeat_album, nullptr);
    SDL_InsertTrayEntryAt(main_menu, -1, nullptr, 0);
    entry_quit = SDL_InsertTrayEntryAt(main_menu, -1, tr_utf8("tray.quit").c_str(), 0);
    SDL_SetTrayEntryCallback(entry_quit, cb_quit, nullptr);
  }

  void update() {
    if (!system_tray) { return; }

    TrayIconState target_state = TrayIconState::STOPPED;
    if (player::is_playing()) {
      target_state = TrayIconState::PLAYING;
    } else if (player::get_playing().has_value()) {
      target_state = TrayIconState::PAUSED;
    }

    if (target_state != current_icon_state) {
      current_icon_state = target_state;
      switch (target_state) {
      case TrayIconState::PLAYING: SDL_SetTrayIcon(system_tray, icon_playing); break;
      case TrayIconState::PAUSED: SDL_SetTrayIcon(system_tray, icon_paused); break;
      case TrayIconState::STOPPED:
      default: SDL_SetTrayIcon(system_tray, icon_stopped); break;
      }
    }

    SDL_SetTrayEntryLabel(entry_play_pause, tr_utf8(player::is_playing() ? "tray.pause" : "tray.play").c_str());
    SDL_SetTrayEntryChecked(entry_shuffle, player::get_shuffle_mode() == player::ShuffleMode::ON);
    auto repeat_mode = player::get_repeat_mode();
    SDL_SetTrayEntryChecked(entry_repeat_off, repeat_mode == player::RepeatMode::OFF);
    SDL_SetTrayEntryChecked(entry_repeat_track, repeat_mode == player::RepeatMode::TRACK);
    SDL_SetTrayEntryChecked(entry_repeat_album, repeat_mode == player::RepeatMode::ALBUM);
  }

  void deinit() {
    if (system_tray) {
      SDL_DestroyTray(system_tray);
      system_tray = nullptr;
    }
    if (icon_stopped) {
      SDL_DestroySurface(icon_stopped);
      icon_stopped = nullptr;
    }
    if (icon_playing) {
      SDL_DestroySurface(icon_playing);
      icon_playing = nullptr;
    }
    if (icon_paused) {
      SDL_DestroySurface(icon_paused);
      icon_paused = nullptr;
    }
    current_icon_state = TrayIconState::NONE;
  }
} // namespace tray
