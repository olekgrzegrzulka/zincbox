#pragma once
#include "../types.hpp"
#include "musicdb.hpp"

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

  struct playing_t {
      size_t collection_id{};
      size_t playlist_id{};
      size_t track_id{};
      bool operator==(const playing_t&) const = default;
  };

  void init();
  void deinit();

  void update();

  void play(playing_t, bool clear_history = true);
  void resume();
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
  std::optional<playing_t> get_playing();

  ShuffleMode get_shuffle_mode();
  void set_shuffle_mode(ShuffleMode);

  RepeatMode get_repeat_mode();
  void set_repeat_mode(RepeatMode);

} // namespace player
