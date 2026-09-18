#pragma once
#include <concepts>
#include <memory>
#include <utility>
#include "common/types.hpp"
#include "core/i_window.hpp"

struct Settings;

namespace zincbox {
  float ui_scale();

  enum InitFlags : u64 { WINDOW = 1, TRAY = 2, MPRIS = 4 };

  template <std::derived_from<IWindow> T> void init(u64 flags) {
    init_INTERNAL(flags, std::move(std::make_unique<T>()));
  }

  void init_INTERNAL(u64, std::unique_ptr<IWindow>);
  void deinit();

  void run();
  void stop();

  void load_state_from_json();
  void apply_loaded_state();
  void save_state_to_json();
  void save_db_to_file();

  IWindow* window();
  Settings& settings();
} // namespace zincbox
