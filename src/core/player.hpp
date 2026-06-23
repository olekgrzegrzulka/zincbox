#pragma once
#include <optional>
#include <vector>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "lib/json.cpp/json.h"

namespace player {
  enum class RepeatMode {
    OFF,
    TRACK,
    ALBUM,
    REPEAT_MODE_SIZE,
  };

  enum class ShuffleMode {
    OFF,
    ON,
    SHUFFLE_MODE_SIZE,
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

  bool play(playing_t, bool clear_queue = true);
  bool play_playlist(size_t collection_id, size_t playlist_id, bool clear_queue = true);
  void enqueue(playing_t, size_t at);
  void remove_from_queue(size_t at);
  void resume();
  void pause();
  void stop();
  void seek_ms(i32 ms);
  void set_volume(float);
  float get_volume();

  void next_track(i32 tries = 10);
  void prev_track(i32 tries = 10);

  i32 get_current_time_ms();
  i32 get_total_duration_ms();
  bool is_playing();
  bool is_at_end();
  std::optional<playing_t> get_playing();
  const std::vector<player::playing_t>& get_playing_queue();
  std::optional<size_t> get_playing_index();
  void set_playing_index(std::optional<size_t>);

  ShuffleMode get_shuffle_mode();
  void set_shuffle_mode(ShuffleMode);

  RepeatMode get_repeat_mode();
  void set_repeat_mode(RepeatMode);

  jt::Json to_json();
  void from_json(const jt::Json&);

  extern const Signal<> signal_on_track_changed;
  extern const Signal<bool /* track_appended_to_back */> signal_on_queue_changed;

} // namespace player
