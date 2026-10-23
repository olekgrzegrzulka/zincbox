#include "core/player/player.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include "common/logger.hpp"
#include "common/utf.hpp"
#include "core/cover_art.hpp"
#include "core/mpris.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/player/audio_engine.hpp"
#include "core/player/queue.hpp"
#include "core/settings.hpp"
#include "zincbox.hpp"

namespace {
  player::AudioEngine engine{};
  float volume = 0.5f;
  i32 crossfade_ms = 0;
  player::EqualizerGains eq_gains{};
  i32 remaining_retry_attempts = 10;
  bool last_direction_was_prev = false;
  i32 display_time_ms = 0;
} // namespace

namespace {

  void notify_active_track_changed(size_t loaded_track_id) {
    db::set_track_playback_error(loaded_track_id, false);
    player::signal_on_track_changed.emit();

    mpris::notify_playback_status_playing();
    mpris::notify_volume(volume);

    auto playing = player::get_playing_track();
    if (playing.has_value() && playing->track_id == loaded_track_id) {
      auto track_opt = db::track_by_id(playing->track_id);
      auto playlist_opt = db::playlist_by_id(playing->playlist_id);
      if (track_opt.has_value() && playlist_opt.has_value()) {
        auto& track = track_opt->get();
        auto& playlist = playlist_opt->get();
        auto originating_album = db::playlist_by_id(track.originating_album_id);

        std::string_view cover_path = playlist.art_file_path;
        if (zincbox::settings().general.cover_preference == Settings::CoverPreference::ALBUM &&
            originating_album.has_value()) {
          cover_path = originating_album->get().art_file_path;
        }

        if (!track.metadata.title.empty() && !track.metadata.artist.empty()) {
          mpris::notify_track_change(track.metadata.title, track.metadata.artist, playlist.name,
                                     static_cast<u64>(track.metadata.duration_ms), std::string(cover_path));
        } else {
          mpris::notify_track_change(track.pretty_name(), "", playlist.name,
                                     static_cast<u64>(track.metadata.duration_ms), std::string(cover_path));
        }
      }
    }
  }

  bool dispatch_play_track(bool use_crossfade) {
    auto playing = player::get_playing_track();
    if (!playing.has_value()) {
      player::stop();
      return true;
    }

    auto track_opt = db::track_by_id(playing->track_id);
    auto playlist_opt = db::playlist_by_id(playing->playlist_id);
    if (!track_opt.has_value() || !playlist_opt.has_value()) { return false; }

    auto& track = track_opt->get();
    engine.push_command(player::CmdPlay{
      .track_id = playing->track_id,
      .file_path = track.file_path,
      .crossfade = use_crossfade,
    });

    display_time_ms = 0;
    return true;
  }

  void start_auto_crossfade(i32 tries) {
    if (tries <= 0) { return; }
    tries = std::min(tries, 50);
    remaining_retry_attempts = tries;
    last_direction_was_prev = false;

    auto next_ti = player::queue::prepare_next_track();
    if (!next_ti.has_value()) { return; }

    auto track_opt = db::track_by_id(next_ti->track_id);
    auto playlist_opt = db::playlist_by_id(next_ti->playlist_id);
    if (!track_opt.has_value() || !playlist_opt.has_value()) {
      player::queue::commit_pending_next();
      start_auto_crossfade(tries - 1);
      return;
    }

    engine.push_command(player::CmdStartAutoCrossfade{
      .track_id = next_ti->track_id,
      .file_path = track_opt->get().file_path,
    });
  }

  void step_next_internal(i32 tries, bool use_crossfade) {
    if (tries <= 0) { return; }
    tries = std::min(tries, 50);
    remaining_retry_attempts = tries;
    last_direction_was_prev = false;

    auto action = player::queue::advance_next();
    if (action == player::queue::NavAction::SEEK_ZERO_AND_PLAY) {
      if (use_crossfade && crossfade_ms > 0) {
        dispatch_play_track(true);
      } else {
        player::seek_ms(0);
        dispatch_play_track(false);
      }
    } else if (action == player::queue::NavAction::PLAY_CURRENT) {
      if (!dispatch_play_track(use_crossfade)) { step_next_internal(tries - 1, use_crossfade); }
    }
  }
} // namespace

namespace player {

  const Signal<> signal_on_track_changed{};
  const Signal<bool> signal_on_queue_changed{};
  const Signal<size_t> signal_on_queue_changed_at{};

  void init() {
    engine.init();
    set_volume(volume);
    set_crossfade(zincbox::settings().playback.crossfade);
    set_equalizer(zincbox::settings().playback.equalizer);
  }

  void deinit() { engine.deinit(); }

  void update() {
    if (crossfade_ms != zincbox::settings().playback.crossfade) {
      set_crossfade(zincbox::settings().playback.crossfade);
    }
    if (eq_gains != zincbox::settings().playback.equalizer) { set_equalizer(zincbox::settings().playback.equalizer); }

    while (auto evt_opt = engine.pop_event()) {
      const auto& evt = *evt_opt;

      if (const auto* loaded = std::get_if<EvtTrackLoaded>(&evt)) {
        notify_active_track_changed(loaded->track_id);
      } else if (const auto* promoted = std::get_if<EvtAutoCrossfadePromoted>(&evt)) {
        queue::commit_pending_next();
        notify_active_track_changed(promoted->track_id);
      } else if (const auto* err = std::get_if<EvtTrackError>(&evt)) {
        out::warn("player::play: decoder returned {} for {}", err->error_code, path_to_utf8(err->file_path));
        db::set_track_playback_error(err->track_id, true);
        if (err->was_auto_crossfade) {
          queue::commit_pending_next();
          start_auto_crossfade(remaining_retry_attempts - 1);
        } else if (last_direction_was_prev) {
          prev_track(remaining_retry_attempts - 1);
        } else {
          step_next_internal(remaining_retry_attempts - 1, true);
        }
      } else if (std::holds_alternative<EvtCrossfadeTrigger>(evt)) {
        start_auto_crossfade(10);
      } else if (std::holds_alternative<EvtTrackEnded>(evt)) {
        step_next_internal(10, false);
      }
    }

    auto current_elem = get_playing();
    if (current_elem.has_value() && current_elem->is_track()) {
      auto cover = zincbox::cover_art::get_full_res(current_elem->track()->track_info.track_id);
      if (cover.has_value() && !cover->empty()) { mpris::notify_cover_change(path_to_utf8(cover.value())); }
    }
  }

  bool play(db::track_info ti, bool clear_q) { return play(QueueTrack(ti), clear_q); }

  bool play(const QueueElement& elem, bool clear_q) {
    last_direction_was_prev = false;
    remaining_retry_attempts = 10;
    queue::set_or_insert_current(elem, clear_q);
    return dispatch_play_track(true);
  }

  bool play_playlist(size_t collection_id, size_t playlist_id, bool clear_q) {
    last_direction_was_prev = false;
    remaining_retry_attempts = 10;
    if (queue::play_playlist(collection_id, playlist_id, clear_q)) { return dispatch_play_track(true); }
    engine.push_command(CmdCancelAutoCrossfade{});
    return true;
  }

  void enqueue(const QueueElement& elem, size_t at) {
    engine.push_command(CmdCancelAutoCrossfade{});
    if (queue::enqueue(elem, at)) { dispatch_play_track(true); }
  }

  std::vector<size_t> add_to_queue(std::span<const db::track_info> tracks, size_t at) {
    engine.push_command(CmdCancelAutoCrossfade{});
    auto [indices, should_play] = queue::add_tracks(tracks, at);
    if (should_play) { dispatch_play_track(true); }
    return indices;
  }

  void remove_from_queue(size_t at) {
    engine.push_command(CmdCancelAutoCrossfade{});
    if (queue::remove_at(at)) { dispatch_play_track(true); }
  }

  void remove_from_queue(std::span<const size_t> indices) {
    engine.push_command(CmdCancelAutoCrossfade{});
    if (queue::remove_indices(indices)) { dispatch_play_track(true); }
  }

  std::vector<size_t> move_queue_tracks(std::span<const size_t> indices, size_t target_index) {
    engine.push_command(CmdCancelAutoCrossfade{});
    return queue::move_tracks(indices, target_index);
  }

  void clear_queue() {
    queue::clear();
    stop();
    set_playing_index(std::nullopt);
    player::signal_on_track_changed.emit();
  }

  void resume() {
    engine.set_user_playing(true);
    engine.push_command(CmdResume{});
    mpris::notify_playback_status_playing();
  }

  void pause() {
    engine.set_user_playing(false);
    engine.push_command(CmdPause{});
    mpris::notify_playback_status_paused();
  }

  void stop() {
    queue::cancel_pending_next();
    engine.set_user_playing(false);
    engine.push_command(CmdStop{});
    mpris::notify_playback_status_stopped();
  }

  void seek_ms(i32 ms) {
    queue::cancel_pending_next();
    engine.push_command(CmdSeek{.position_ms = ms});
    display_time_ms = ms;
    mpris::notify_seeked(ms);
  }

  void set_volume(float v) {
    volume = std::clamp(v, 0.0f, 1.0f);
    engine.push_command(CmdSetVolume{.volume = volume});
    mpris::notify_volume(volume);
  }

  float get_volume() { return volume; }

  void set_crossfade(i32 ms) {
    crossfade_ms = std::max(0, ms);
    zincbox::settings().playback.crossfade = crossfade_ms;
    engine.push_command(CmdSetCrossfade{.crossfade_ms = crossfade_ms});
  }

  i32 get_crossfade() { return crossfade_ms; }

  void set_equalizer(const std::array<float, 10>& gains_db) {
    eq_gains = gains_db;
    zincbox::settings().playback.equalizer = eq_gains;
    engine.push_command(CmdSetEqualizer{.gains = eq_gains});
  }

  const std::array<float, 10>& get_equalizer() { return eq_gains; }

  void next_track(i32 tries) { step_next_internal(tries, true); }

  void prev_track(i32 tries) {
    if (tries <= 0) { return; }
    tries = std::min(tries, 50);
    remaining_retry_attempts = tries;
    last_direction_was_prev = true;

    auto action = queue::step_prev(get_current_time_ms());
    if (action == queue::NavAction::SEEK_ZERO) {
      if (crossfade_ms > 0) {
        dispatch_play_track(true);
      } else {
        seek_ms(0);
      }
    } else if (action == queue::NavAction::PLAY_CURRENT) {
      if (!dispatch_play_track(true)) { prev_track(tries - 1); }
    }
  }

  i32 get_current_time_ms() { return engine.get_current_time_ms(); }

  i32 get_display_time_ms() {
    if (!engine.get_is_seeking()) { display_time_ms = engine.get_current_time_ms(); }
    return display_time_ms;
  }

  i32 get_total_duration_ms() { return engine.get_total_duration_ms(); }

  bool is_playing() { return engine.is_playing(); }

  bool is_at_end() { return engine.is_at_end(); }

  std::optional<QueueElement> get_playing() { return queue::get_playing(); }

  std::optional<db::track_info> get_playing_track() { return queue::get_playing_track(); }

  const std::vector<QueueElement>& get_playing_queue() { return queue::get_elements(); }

  FlattenedQueue get_flattened_queue() {
    FlattenedQueue ret;
    ret.tracks.reserve(get_playing_queue().size());

    const auto& queue = get_playing_queue();
    auto playing_index = queue::get_playing_index();

    for (size_t i = 0; i < queue.size(); i += 1) {
      auto& elem = queue[i];
      size_t curr_size = ret.tracks.size();

      if (elem.is_track()) {
        ret.tracks.emplace_back(elem.track()->track_info);
        if (playing_index == i) { ret.playing_index = curr_size; }
      } else if (elem.is_playlist()) {
        auto tracks = elem.playlist()->get_playback();
        for (auto& t : tracks) {
          ret.tracks.emplace_back(t);
        }
        if (playing_index == i) {
          size_t internal_index = elem.playlist()->get_playback_index().value_or(0);
          ret.playing_index = curr_size + internal_index;
        }
      } else if (elem.is_collection()) {
        auto tracks = elem.collection()->get_playback();
        for (auto& t : tracks) {
          ret.tracks.emplace_back(t);
        }
        if (playing_index == i) {
          size_t internal_index = elem.collection()->get_playback_index().value_or(0);
          ret.playing_index = curr_size + internal_index;
        }
      }
    }
    return ret;
  }

  std::optional<size_t> get_playing_index() { return queue::get_playing_index(); }

  void set_playing_index(std::optional<size_t> i) {
    if (queue::set_playing_index(i)) { dispatch_play_track(true); }
  }

  ShuffleMode get_shuffle_mode() { return queue::get_shuffle_mode(); }

  void set_shuffle_mode(ShuffleMode s) {
    if (queue::set_shuffle_mode(s)) {
      engine.push_command(CmdCancelAutoCrossfade{});
      mpris::notify_shuffle(s == ShuffleMode::ON);
    }
  }

  RepeatMode get_repeat_mode() { return queue::get_repeat_mode(); }

  void set_repeat_mode(RepeatMode r) {
    if (queue::set_repeat_mode(r)) {
      engine.push_command(CmdCancelAutoCrossfade{});
      mpris::notify_loop_status(static_cast<i32>(r));
    }
  }

} // namespace player
