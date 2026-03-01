#pragma once
#include "types.hpp"

namespace musicdb {
  struct Track;
  struct Album;
  class Collection;

  using collection_id_t = size_t;
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

  struct now_playing_t {
      musicdb::track_id_t track_id = std::numeric_limits<size_t>::max();
      musicdb::album_id_t album_id = std::numeric_limits<size_t>::max();
      musicdb::collection_id_t collection_id = std::numeric_limits<size_t>::max();
      bool operator==(const now_playing_t&) const = default;
  };

  void init();
  void deinit();

  void update();

  void play(bool clear_history = true);
  void play(now_playing_t, bool clear_history = true);
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
  std::optional<now_playing_t> get_playing();

  ShuffleMode get_shuffle_mode();
  void set_shuffle_mode(ShuffleMode);

  RepeatMode get_repeat_mode();
  void set_repeat_mode(RepeatMode);

} // namespace player
