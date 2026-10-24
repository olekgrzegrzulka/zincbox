#pragma once
#include "common/types.hpp"

#ifdef ZINCBOX_HAS_GUI
namespace zincbox {
  class SDL3Window;
}
#endif

struct Settings;

namespace zincbox {
  float ui_scale();
  i32 font_size_xs();
  i32 font_size_sm();
  i32 font_size();
  i32 font_size_lg();
  i32 font_size_xl();

  enum InitFlags : u8 { WINDOW = 1, TRAY = 2, MPRIS = 4 };

  void init(u64 flags);
  void deinit();

  void run();
  void stop();

  void load_state_from_json();
  void apply_loaded_state();
  void save_state_to_json();
  void load_db_from_file();
  void save_db_to_file();

#ifdef ZINCBOX_HAS_GUI
  SDL3Window* window();
#endif
  Settings& settings();
} // namespace zincbox
