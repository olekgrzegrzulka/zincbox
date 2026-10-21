#pragma once
#include <optional>
#include <span>
#include <vector>
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "core/queue_element.hpp"

namespace player::queue {

  enum class NavAction : u8 { NONE, PLAY_CURRENT, SEEK_ZERO, SEEK_ZERO_AND_PLAY };

  bool set_or_insert_current(const QueueElement& elem, bool clear_queue);
  bool play_playlist(size_t collection_id, size_t playlist_id, bool clear_queue);
  bool enqueue(const QueueElement& elem, size_t at);
  std::pair<std::vector<size_t>, bool> add_tracks(std::span<const db::track_info> tracks, size_t at);
  bool remove_at(size_t at);
  bool remove_indices(std::span<const size_t> indices);
  std::vector<size_t> move_tracks(std::span<const size_t> indices, size_t target_index);
  void clear();

  std::optional<db::track_info> prepare_next_track();
  bool commit_pending_next();
  void cancel_pending_next();
  bool has_pending_next();

  NavAction advance_next();
  NavAction step_prev(i32 current_time_ms);

  std::optional<QueueElement> get_playing();
  std::optional<db::track_info> get_playing_track();
  const std::vector<QueueElement>& get_elements();
  std::optional<size_t> get_playing_index();
  bool set_playing_index(std::optional<size_t> index);

  ShuffleMode get_shuffle_mode();
  bool set_shuffle_mode(ShuffleMode mode);

  RepeatMode get_repeat_mode();
  bool set_repeat_mode(RepeatMode mode);

} // namespace player::queue
