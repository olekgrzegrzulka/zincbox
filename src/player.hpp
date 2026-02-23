#pragma once
#include <string>
#include "types.hpp"

namespace player {
  void init();
  void deinit();

  void play();
  void play(const std::string& path);
  void pause();
  void stop();
  void seek_ms(i32 ms);
  i32 get_current_time_ms();
  i32 get_total_duration_ms();
  bool is_playing();
} // namespace player
