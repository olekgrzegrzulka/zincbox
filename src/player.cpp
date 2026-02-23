#include "player.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include "../lib/miniaudio/miniaudio.h"
#include "debug.hpp"
#include "musicdb.hpp"
#include "random.hpp"

ma_engine engine{};
ma_sound sound{};
ma_device device{};

std::optional<musicdb::track_id_t> track_id{};
i32 total_duration_ms{};
float volume = 0.5f;

player::ShuffleMode shuffle_mode = player::ShuffleMode::OFF;
player::RepeatMode repeat_mode = player::RepeatMode::OFF;

std::vector<musicdb::track_id_t> tracks_history{};
std::optional<size_t> tracks_history_current_index{};

void device_data_callback(ma_device* /*pDevice*/, void* /*pOutput*/, const void* /*pInput*/, ma_uint32 /*frameCount*/) {
}

void player::init() {
  auto device_config = ma_device_config_init(ma_device_type_playback);
  device_config.dataCallback = device_data_callback;

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

void player::update() {
  // debug_log("tracks_history: ", tracks_history);
  // debug_log("index         : ", tracks_history_current_index.value_or(2137));
  if (is_at_end()) {
    if (auto curr_track = get_track(); curr_track.has_value()) {
      next_track();
    }
  }
}

void player::play(bool clear_history) {
  if (clear_history) {
    tracks_history.clear();
    tracks_history_current_index = std::nullopt;
  }
  ma_sound_start(&sound);
  ma_sound_set_volume(&sound, volume);
}

void player::play(musicdb::track_id_t track_id_, bool clear_history) {
  if (clear_history) {
    tracks_history.clear();
    tracks_history_current_index = std::nullopt;
  }
  track_id = track_id_;

  ma_engine_stop(&engine);
  ma_sound_uninit(&sound);

  ma_result result;

  result = ma_sound_init_from_file(&engine,
                                   musicdb::get_tracks()[track_id.value()].path.c_str(),
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

  if (tracks_history.size() == 0) {
    ensure(tracks_history_current_index == std::nullopt);
    tracks_history = {track_id_};
    tracks_history_current_index = 0;
  } else if (tracks_history_current_index.has_value()) {
    if (tracks_history_current_index == tracks_history.size() - 1) {
      tracks_history.emplace_back(track_id_);
      *tracks_history_current_index += 1;
    } else {
      tracks_history.resize(tracks_history_current_index.value());
      tracks_history.emplace_back(track_id_);
      // *tracks_history_current_index += 1;
    }
  }

  if (tracks_history.size() >= (4096 - 1) && tracks_history_current_index.value_or(0) == tracks_history.size() - 1) {
    std::copy(tracks_history.begin() + 2048, tracks_history.end(), tracks_history.begin());
    tracks_history.resize(tracks_history.size() - 2048);
    tracks_history_current_index = tracks_history.size() - 1;
  }
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

void player::next_track() {
  static Random rng = Random();

  using namespace musicdb;

  auto is_in_tracks_history = [&](track_id_t track_id, i32 last_n_to_check) {
    for (i32 i = std::max(0, (i32)tracks_history.size() - last_n_to_check); i < (i32)tracks_history.size(); i += 1) {
      if (track_id == tracks_history[i]) {
        return true;
      }
    }
    return false;
  };

  if (!track_id.has_value()) { return; }

  if (repeat_mode == RepeatMode::TRACK) {
    seek_ms(0);
    play(false);
    return;
  }

  if (shuffle_mode == ShuffleMode::OFF) {
    if (repeat_mode == RepeatMode::OFF) {
      auto& track_curr = musicdb::get_tracks()[track_id.value()];
      play(track_curr.next_track_id, false);
    } else if (repeat_mode == RepeatMode::ALBUM && track_id.has_value()) {
      auto& track_curr = musicdb::get_tracks()[track_id.value()];
      auto& track_next = musicdb::get_tracks()[track_curr.next_track_id];
      if (track_next.album_id == track_curr.album_id) {
        play(track_next.track_id, false);
      } else {
        track_id_t track_first = musicdb::get_albums()[track_curr.album_id].first_track_id;
        play(track_first, false);
      }
    }
  } else if (shuffle_mode == ShuffleMode::ON) {
    if (repeat_mode == RepeatMode::OFF) {
      i32 tries_left = 32;
      track_id_t random_track_id = 0;
      while (tries_left-- > 0) {
        random_track_id = rng.next<track_id_t>(0, musicdb::get_tracks().size() - 1);
        bool found = is_in_tracks_history(random_track_id, 20);
        if (!found) { break; }
      }
      play(random_track_id, false);
    } else if (repeat_mode == RepeatMode::ALBUM && track_id.has_value()) {
      auto& track_curr = musicdb::get_tracks()[track_id.value()];
      auto& album = musicdb::get_album(track_curr.album_id);

      i32 tries_left = 32;
      musicdb::track_id_t random_track_id = 0;
      while (tries_left-- > 0) {
        i32 random_album_track_index = rng.next<size_t>(0, album.track_ids.size() - 1);
        random_track_id = album.track_ids[random_album_track_index];
        bool found = is_in_tracks_history(random_track_id, album.track_ids.size() / 2);
        if (!found) { break; }
      }
      play(random_track_id, false);
    }
  }
}
void player::prev_track() {
  if (!tracks_history_current_index.has_value()) { return; }
  if (tracks_history.size() == 1) {
    seek_ms(0);
  } else if (tracks_history.size() > 1) {
    if (*tracks_history_current_index > 0) {
      *tracks_history_current_index -= 1;
    }
    play(tracks_history[*tracks_history_current_index], false);
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

std::optional<musicdb::track_id_t> player::get_track() {
  return track_id;
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
