#pragma once
#include "types.hpp"

namespace musicdb {
  struct Track;
  struct Album;

  using track_id_t = size_t;
  using album_id_t = size_t;
} // namespace musicdb

namespace player {
  enum class RepeatMode {
    OFF,
    TRACK,
    ALBUM,
  };

  enum class ShuffleMode {
    OFF,
    ON,
  };

  void init();
  void deinit();

  void update();

  void play(bool clear_history = true);
  void play(musicdb::track_id_t, bool clear_history = true);
  void pause();
  void stop();
  void seek_ms(i32 ms);
  void set_volume(float);
  float get_volume();

  void next_track();
  void prev_track();

  i32 get_current_time_ms();
  i32 get_total_duration_ms();
  bool is_playing();
  bool is_at_end();
  std::optional<musicdb::track_id_t> get_track();

  ShuffleMode get_shuffle_mode();
  void set_shuffle_mode(ShuffleMode);

  RepeatMode get_repeat_mode();
  void set_repeat_mode(RepeatMode);

} // namespace player
