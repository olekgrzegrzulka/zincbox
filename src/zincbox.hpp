#pragma once
#include <concepts>
#include <memory>
#include <utility>
#include "common/types.hpp"
#include "ui/sdl3_window.hpp"

namespace zincbox {
  class SDL3Window;
}

struct Settings;

namespace zincbox {
  float ui_scale();

  enum InitFlags : u64 { WINDOW = 1, TRAY = 2, MPRIS = 4 };

  void init(u64 flags);
  void deinit();

  void run();
  void stop();

  void load_state_from_json();
  void apply_loaded_state();
  void save_state_to_json();
  void save_db_to_file();

  SDL3Window* window();
  Settings& settings();
} // namespace zincbox
