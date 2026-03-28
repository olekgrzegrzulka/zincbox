#include "player.hpp"
#include <algorithm>
#include <optional>
#include <vector>
#include "../../lib/miniaudio/miniaudio.h"
#include "../random.hpp"
#include "../utf.hpp"
#include "musicdb.hpp"

Random rng{};

ma_engine engine{};
ma_sound sound{};
ma_device device{};

i32 total_duration_ms{};
std::vector<player::playing_t> playing_queue{};
std::optional<size_t> playing_index{};
float volume = 0.5f;
player::ShuffleMode shuffle_mode = player::ShuffleMode::OFF;
player::RepeatMode repeat_mode = player::RepeatMode::OFF;

void player::init() {
  auto device_config = ma_device_config_init(ma_device_type_playback);

  ma_result result = ma_device_init(NULL, &device_config, &device);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }

  ma_device_start(&device);

  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }
}

void player::deinit() {
  ma_engine_uninit(&engine);
  ma_device_uninit(&device);
}

void play_track() {
  auto playing = player::get_playing();
  if (!playing.has_value()) { return; }
  auto track = db::track_by_id(playing->track_id);

  ma_engine_stop(&engine);
  ma_sound_uninit(&sound);

  ma_result result;

  result = ma_sound_init_from_file(&engine,
                                   utf32_to_utf8(track->get().path).c_str(),
                                   MA_SOUND_FLAG_NO_PITCH,
                                   NULL,
                                   NULL,
                                   &sound);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }

  float ret;
  result = ma_sound_get_length_in_seconds(&sound, &ret);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }
  total_duration_ms = ret * 1000.0f;

  result = ma_sound_start(&sound);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }
  ma_sound_set_volume(&sound, volume);
  result = ma_engine_start(&engine);
  if (result != MA_SUCCESS) {
    ma_sound_stop(&sound);
    debug_warn(result);
    return;
  }
}

void player::update() {
  if (is_at_end() && get_playing().has_value()) {
    next_track();
  }
}

void player::play(playing_t play, bool clear_history) {
  playing_queue.emplace_back(player::playing_t{
    .collection_id = play.collection_id,
    .playlist_id = play.playlist_id,
    .track_id = play.track_id,
  });

  if (clear_history || playing_queue.size() == 0) {
    playing_queue = {play};
    playing_index = 0;
  } else {
    if (!playing_index.has_value()) {
      playing_queue.emplace_back(play);
      playing_index = playing_queue.size() - 1;
    } else if (playing_index.value() > playing_queue.size()) {
      playing_queue.emplace_back(play);
      playing_index = playing_queue.size() - 1;
    } else {
      playing_queue[*playing_index + 1] = play;
      (*playing_index) += 1;
    }
  }

  play_track();
}

void player::resume() {
  ma_sound_start(&sound);
}

void player::pause() {
  ma_sound_stop(&sound);
}

void player::stop() {
  ma_sound_seek_to_second(&sound, 0);
  ma_sound_stop(&sound);
}

void player::seek_ms(i32 ms) {
  ma_sound_seek_to_second(&sound, ms * 0.001f);
}

void player::set_volume(float v) {
  volume = std::clamp(v, 0.0f, 1.0f);
  ma_sound_set_volume(&sound, volume);
}

float player::get_volume() {
  return volume;
}

bool is_in_tracks_history(size_t track_id, i32 from_index, i32 to_index) {
  if (playing_queue.size() == 0) { return false; }
  from_index = std::max(from_index, 0);
  to_index = std::min(to_index, (i32)playing_queue.size() - 1);
  if (from_index > to_index) { return false; }
  for (i32 i = from_index; i <= to_index; i += 1) {
    if (track_id == playing_queue[i].track_id) { return true; }
  }
  return false;
};

void player::next_track() {
  if (!playing_index.has_value()) { return; }

  auto playing = playing_queue[*playing_index];
  auto playlist = db::playlist_by_id(playing.playlist_id);
  auto collection = db::collection_by_id(playing.collection_id);
  if (!playlist.has_value()) { return; }

  if (repeat_mode == RepeatMode::TRACK) {
    seek_ms(0);
  } else if (shuffle_mode == ShuffleMode::OFF) {
    auto next_track_id = playlist->get().next_track_id(playing.track_id);
    if (next_track_id.has_value()) {
      play(player::playing_t{
             .collection_id = playing.collection_id,
             .playlist_id = playing.playlist_id,
             .track_id = next_track_id.value(),
           },
           false);
    } else if (repeat_mode == RepeatMode::OFF) {
      auto next_playlist_id = collection.value().get().next_playlist_id(playing.playlist_id);
      if (next_playlist_id.has_value()) {
        auto next_playlist = db::playlist_by_id(next_playlist_id.value());
        if (!next_playlist.has_value()) { return; }
        if (next_playlist.value().get().track_ids.size() == 0) { return; }
        play(player::playing_t{
               .collection_id = playing.collection_id,
               .playlist_id = next_playlist_id.value(),
               .track_id = next_playlist.value().get().track_ids[0],
             },
             false);
      }
    } else if (repeat_mode == RepeatMode::ALBUM) {
      if (playlist->get().track_ids.size() == 0) {
        debug_warn("player::play: empty playlist");
        return;
      }
      play(player::playing_t{
             .collection_id = playing.collection_id,
             .playlist_id = playing.playlist_id,
             .track_id = playlist->get().track_ids[0],
           },
           false);
    } else {
      debug_warn("player::play: unknown RepeatMode enum value");
      return;
    }
  } else if (shuffle_mode == ShuffleMode::ON) {
    if (repeat_mode == RepeatMode::OFF) {
      size_t rand_playlist_id = 0;
      size_t rand_track_id = 0;
      for (i32 tries = 10; tries > 0; tries -= 1) {
        rand_playlist_id = rng.pick(std::span{db::collection_by_id(playing.collection_id)->get().playlist_ids});
        rand_track_id = rng.pick(std::span{db::playlist_by_id(rand_playlist_id)->get().track_ids});

        if (!is_in_tracks_history(rand_track_id, playing_index.value_or(0) - 20, playing_index.value_or(0))) {
          break;
        }
      }

      play(player::playing_t{
             .collection_id = playing.collection_id,
             .playlist_id = rand_playlist_id,
             .track_id = rand_track_id,
           },
           false);
    } else if (repeat_mode == RepeatMode::ALBUM) {
      size_t rand_track_id = 0;
      for (i32 tries = 10; tries > 0; tries -= 1) {
        rand_track_id = rng.pick(std::span{db::playlist_by_id(playing.playlist_id)->get().track_ids});
        i32 last_tracks_to_check = std::clamp((i32)db::playlist_by_id(playing.playlist_id)->get().track_ids.size() / 2, 0, 30);
        if (!is_in_tracks_history(rand_track_id, playing_index.value_or(0) - last_tracks_to_check, playing_index.value_or(0))) {
          break;
        }
      }
      play(player::playing_t{
             .collection_id = playing.collection_id,
             .playlist_id = playing.playlist_id,
             .track_id = rand_track_id,
           },
           false);
    } else {
      debug_warn("player::play: unknown RepeatMode enum value");
    }

  } else {
    debug_warn("player::play: unknown ShuffleMode or RepeatMode enum value");
    return;
  }

  // debug_warn("player::next_track: couldn't find track with track_id = ", playlist_track_ids[*playing_index]);
  // debug_warn("player::next_track: couldn't find track on playlist: track_id = ",play.track_id, ", playlist_id: ", play.playlist_id, ", collection_id: ", play.collection_id);
}

void player::prev_track() {
  if (playing_index.has_value() && playing_index.value() > 0) {
    playing_index = playing_index.value() - 1;
    play_track();
  }
}

i32 player::get_current_time_ms() {
  float ret;
  auto result = ma_sound_get_cursor_in_seconds(&sound, &ret);
  if (result != MA_SUCCESS) { return 0; }
  return ret * 1000.0f;
}

i32 player::get_total_duration_ms() {
  return total_duration_ms;
}

bool player::is_playing() {
  return ma_sound_is_playing(&sound);
}

bool player::is_at_end() {
  return ma_sound_at_end(&sound);
}

std::optional<player::playing_t> player::get_playing() {
  if (!playing_index.has_value()) { return std::nullopt; }
  return playing_queue[playing_index.value()];
}

player::ShuffleMode player::get_shuffle_mode() {
  return shuffle_mode;
}
void player::set_shuffle_mode(ShuffleMode s) {
  shuffle_mode = s;
}

player::RepeatMode player::get_repeat_mode() {
  return repeat_mode;
}
void player::set_repeat_mode(RepeatMode r) {
  repeat_mode = r;
}
