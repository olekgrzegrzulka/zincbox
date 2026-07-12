#include "player.hpp"
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <vector>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/random.hpp"
#include "common/utf.hpp"
#include "core/mpris.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "lib/miniaudio/miniaudio.h"

static Random rng{};

static ma_engine engine{};
static ma_sound sound{};
static ma_device device{};

static i32 total_duration_ms{};
static std::vector<db::track_info> playing_queue{};
static std::optional<size_t> playing_index{};
static float volume = 0.5f;
static player::ShuffleMode shuffle_mode = player::ShuffleMode::OFF;
static player::RepeatMode repeat_mode = player::RepeatMode::OFF;

const Signal<> player::signal_on_track_changed{};
const Signal<bool /* track_appended_to_back */> player::signal_on_queue_changed{};

void player::init() {
  auto device_config = ma_device_config_init(ma_device_type_playback);

  ma_result result = ma_device_init(NULL, &device_config, &device);
  if (result != MA_SUCCESS) {
    out::critical("couldn't initialize player, error {}", (i32)result);
    return;
  }

  ma_device_start(&device);

  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    out::critical("couldn't initialize player, error {}", (i32)result);
    return;
  }
}

void player::deinit() {
  ma_engine_uninit(&engine);
  ma_device_uninit(&device);
}

bool play_track() {
  auto playing = player::get_playing();
  if (!playing.has_value()) {
    player::stop();
    return true;
  }
  auto& track = db::track_by_id(playing->track_id)->get();
  auto& playlist = db::playlist_by_id(playing->playlist_id)->get();

  ma_engine_stop(&engine);
  ma_sound_uninit(&sound);

  ma_result result;

  result =
    ma_sound_init_from_file(&engine, utf32_to_utf8(track.path).c_str(), MA_SOUND_FLAG_NO_PITCH, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    out::warn("player::play: ma_sound_init_from_file returned {} for {}", (i32)result,
                     utf32_to_utf8(track.path));
    db::set_track_playback_error(playing->track_id, true);
    return false;
  }

  float ret;
  result = ma_sound_get_length_in_seconds(&sound, &ret);
  if (result != MA_SUCCESS) {
    out::warn("player::play: ma_sound_get_length_in_seconds returned {} for {}", (i32)result,
                     utf32_to_utf8(track.path));
    db::set_track_playback_error(playing->track_id, true);
    return false;
  }
  total_duration_ms = ret * 1000.0f;

  result = ma_sound_start(&sound);
  if (result != MA_SUCCESS) {
    out::warn("player::play: ma_sound_start returned {} for {}", (i32)result, utf32_to_utf8(track.path));
    db::set_track_playback_error(playing->track_id, true);
    return false;
  }
  ma_sound_set_volume(&sound, volume);
  result = ma_engine_start(&engine);
  if (result != MA_SUCCESS) {
    ma_sound_stop(&sound);
    out::warn("player::play: ma_engine_start returned {} for {}", (i32)result, utf32_to_utf8(track.path));
    db::set_track_playback_error(playing->track_id, true);
    return false;
  }

  player::signal_on_track_changed.emit();

  mpris::notify_playback_status_playing();
  mpris::notify_volume(volume);
  if (!track.title.empty() && !track.artist.empty()) {
    mpris::notify_track_change(utf32_to_utf8(track.title), utf32_to_utf8(track.artist), utf32_to_utf8(playlist.name),
                               (u64)track.length_seconds * 1000, utf32_to_utf8(playlist.art_file_path));
  } else {
    mpris::notify_track_change(utf32_to_utf8(track.pretty_name()), "", utf32_to_utf8(playlist.name),
                               (u64)track.length_seconds * 1000, utf32_to_utf8(playlist.art_file_path));
  }

  db::set_track_playback_error(playing->track_id, false);
  return true;
}

void player::update() {
  if (is_at_end() && get_playing().has_value()) { next_track(); }
}

bool player::play(db::track_info play, bool clear_queue) {
  if (clear_queue || playing_queue.size() == 0) {
    playing_queue = {play};
    playing_index = 0;
    signal_on_queue_changed.emit(false);
  } else if (!playing_index.has_value() || playing_index.value() >= playing_queue.size() - 1) {
    playing_queue.emplace_back(play);
    playing_index = playing_queue.size() - 1;
    signal_on_queue_changed.emit(true);
  } else {
    playing_queue[*playing_index + 1] = play;
    (*playing_index) += 1;
    signal_on_queue_changed.emit(false);
  }

  return play_track();
}

bool player::play_playlist(size_t collection_id, size_t playlist_id, bool clear_queue) {
  auto playlist = db::playlist_by_id(playlist_id);
  if (!playlist.has_value()) { return true; }
  auto track_ids = playlist->get().get_track_ids();
  if (track_ids.size() == 0) { return true; }

  bool do_play_track = clear_queue || playing_queue.size() == 0;

  if (clear_queue) {
    playing_queue.clear();
    for (size_t track_id : track_ids) {
      playing_queue.emplace_back(db::track_info{
        .collection_id = collection_id,
        .playlist_id = playlist_id,
        .track_id = track_id,
      });
    }
    playing_index = 0;
    signal_on_queue_changed.emit(false);
  } else {
    size_t insert_at = playing_index.has_value() ? playing_index.value() + 1 : playing_queue.size();
    for (size_t i = 0; i < track_ids.size(); i += 1) {
      auto track_id = track_ids[i];
      db::track_info play{
        .collection_id = collection_id,
        .playlist_id = playlist_id,
        .track_id = track_id,
      };
      enqueue(play, insert_at + i - 1);
    }
  }
  if (do_play_track) {
    return play_track();
  } else {
    return true;
  }
}

void player::enqueue(db::track_info play, size_t at) {
  if (at >= playing_queue.size()) {
    playing_queue.emplace_back(play);
    signal_on_queue_changed.emit(true);
  } else {
    playing_queue.emplace(playing_queue.begin() + at + 1, play);
    signal_on_queue_changed.emit(false);
  }

  if (!playing_index.has_value()) {
    playing_index = playing_queue.size() - 1;
    play_track();
  }
}

void player::remove_from_queue(size_t at) {
  bool current_track_removed = at == playing_index;
  if (at >= playing_queue.size()) { return; }
  if (playing_index.has_value() && playing_index <= at) {
    playing_queue.erase(playing_queue.begin() + at);
  } else if (playing_index.has_value() && playing_index > at) {
    playing_queue.erase(playing_queue.begin() + at);
    playing_index = playing_index.value() - 1;
  }

  if (playing_queue.size() == 0) {
    playing_index = std::nullopt;
  } else if (playing_index.has_value()) {
    playing_index = std::min(playing_index.value(), playing_queue.size() - 1);
  }
  if (current_track_removed) { play_track(); }
}

void player::clear_queue() {
  playing_queue.clear();
  playing_index = std::nullopt;
  signal_on_queue_changed.emit(false);
  stop();
}

void player::resume() {
  ma_sound_start(&sound);
  mpris::notify_playback_status_playing();
}

void player::pause() {
  ma_sound_stop(&sound);
  mpris::notify_playback_status_paused();
}

void player::stop() {
  ma_sound_seek_to_second(&sound, 0);
  ma_sound_stop(&sound);
  mpris::notify_playback_status_stopped();
}

void player::seek_ms(i32 ms) {
  ma_sound_seek_to_second(&sound, ms * 0.001f);
  mpris::notify_seeked(ms);
}

void player::set_volume(float v) {
  volume = std::clamp(v, 0.0f, 1.0f);
  ma_sound_set_volume(&sound, volume);
  mpris::notify_volume(v);
}

float player::get_volume() { return volume; }

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

void player::next_track(i32 tries) {
  if (tries <= 0) { return; }
  if (!playing_index.has_value()) { return; }

  if (repeat_mode == RepeatMode::TRACK) {
    seek_ms(0);
    play_track();
    return;
  }

  if (playing_index.value() < playing_queue.size() - 1) {
    playing_index = playing_index.value() + 1;
    if (!play_track()) { next_track(tries - 1); }
    return;
  }

  auto playing = playing_queue[*playing_index];
  auto playlist = db::playlist_by_id(playing.playlist_id);
  auto collection = db::collection_by_id(playing.collection_id);
  if (!playlist.has_value()) { return; }

  if (repeat_mode == RepeatMode::TRACK) {
    seek_ms(0);
  } else if (shuffle_mode == ShuffleMode::OFF) {
    auto next_track_id = playlist->get().next_track_id(playing.track_id);
    if (next_track_id.has_value()) {
      bool result = play(
        db::track_info{
          .collection_id = playing.collection_id,
          .playlist_id = playing.playlist_id,
          .track_id = next_track_id.value(),
        },
        false);
      if (!result) {
        next_track(tries - 1);
        return;
      }
    } else if (repeat_mode == RepeatMode::OFF) {
      auto next_playlist_id = collection->get().next_playlist_id(playing.playlist_id);
      if (next_playlist_id.has_value()) {
        auto next_playlist = db::playlist_by_id(next_playlist_id.value());
        if (!next_playlist.has_value()) { return; }
        if (next_playlist->get().get_tracks_count() == 0) { return; }
        bool result = play(
          db::track_info{
            .collection_id = playing.collection_id,
            .playlist_id = next_playlist_id.value(),
            .track_id = next_playlist->get().get_track_ids()[0],
          },
          false);
        if (!result) {
          next_track(tries - 1);
          return;
        }
      }
    } else if (repeat_mode == RepeatMode::ALBUM) {
      if (playlist->get().get_tracks_count() == 0) {
        out::debug_warn("player::play: empty playlist");
        return;
      }
      bool result = play(
        db::track_info{
          .collection_id = playing.collection_id,
          .playlist_id = playing.playlist_id,
          .track_id = playlist->get().get_track_ids()[0],
        },
        false);
      if (!result) {
        next_track(tries - 1);
        return;
      }
    } else {
      out::debug_warn("player::play: unknown RepeatMode enum value");
      return;
    }
  } else if (shuffle_mode == ShuffleMode::ON) {
    if (repeat_mode == RepeatMode::OFF) {
      size_t rand_playlist_id = 0;
      size_t rand_track_id = 0;
      for (i32 shuffle_tries = 10; shuffle_tries > 0; shuffle_tries -= 1) {
        auto& playlist_ids = db::collection_by_id(playing.collection_id)->get().playlist_ids();
        rand_playlist_id = rng.pick(std::span(playlist_ids));
        i32 collection_track_count = 0;
        for (size_t playlist_id : playlist_ids) {
          collection_track_count += db::playlist_by_id(playlist_id)->get().get_tracks_count();
        }
        rand_track_id = rng.pick(std::span{db::playlist_by_id(rand_playlist_id)->get().get_track_ids()});
        i32 last_tracks_to_check = std::clamp(collection_track_count / 2, 0, 30);
        if (!is_in_tracks_history(rand_track_id, playing_index.value_or(0) - last_tracks_to_check,
                                  playing_index.value_or(0))) {
          break;
        }
      }

      bool result = play(
        db::track_info{
          .collection_id = playing.collection_id,
          .playlist_id = rand_playlist_id,
          .track_id = rand_track_id,
        },
        false);
      if (!result) {
        next_track(tries - 1);
        return;
      }
    } else if (repeat_mode == RepeatMode::ALBUM) {
      size_t rand_track_id = 0;
      for (i32 shuffle_tries = 10; shuffle_tries > 0; shuffle_tries -= 1) {
        rand_track_id = rng.pick(std::span{db::playlist_by_id(playing.playlist_id)->get().get_track_ids()});
        i32 last_tracks_to_check =
          std::clamp((i32)db::playlist_by_id(playing.playlist_id)->get().get_tracks_count() / 2, 0, 30);
        if (!is_in_tracks_history(rand_track_id, playing_index.value_or(0) - last_tracks_to_check,
                                  playing_index.value_or(0))) {
          break;
        }
      }
      bool result = play(
        db::track_info{
          .collection_id = playing.collection_id,
          .playlist_id = playing.playlist_id,
          .track_id = rand_track_id,
        },
        false);
      if (!result) {
        next_track(tries - 1);
        return;
      }
    } else {
      out::debug_warn("player::play: unknown RepeatMode enum value");
    }

  } else {
    out::debug_warn("player::play: unknown ShuffleMode/RepeatMode enum value");
    return;
  }

  // debug_warn("player::next_track: couldn't find track with track_id = ",
  // playlist_track_ids[*playing_index]); debug_warn("player::next_track: couldn't find
  // track on playlist: track_id = ",play.track_id, ", playlist_id: ", play.playlist_id,
  // ", collection_id: ", play.collection_id);
}

void player::prev_track(i32 tries) {
  if (tries <= 0) { return; }
  if (!playing_index.has_value()) { return; }
  std::optional<db::track_info> play_prev;

  if (repeat_mode == RepeatMode::TRACK) {
    seek_ms(0);
    return;
  }

  if (playing_index.value() != 0) {
    playing_index = playing_index.value() - 1;
    if (!play_track()) {
      prev_track(tries - 1);
      return;
    }
    return;
  }

  auto& playing = playing_queue[playing_index.value()];
  auto& collection = db::collection_by_id(playing.collection_id)->get();
  auto& playlist = db::playlist_by_id(playing.playlist_id)->get();
  auto prev_playlist_id = collection.prev_playlist_id(playing.playlist_id);
  auto prev_track_id = playlist.prev_track_id(playing_queue[playing_index.value()].track_id);

  if (prev_track_id.has_value()) {
    play_prev = db::track_info{
      .collection_id = playing.collection_id,
      .playlist_id = playing.playlist_id,
      .track_id = prev_track_id.value(),
    };
  } else if (repeat_mode == RepeatMode::OFF) {
    if (prev_playlist_id.has_value()) {

      auto prev_playlist = db::playlist_by_id(prev_playlist_id.value());
      if (!prev_playlist.has_value()) { return; }

      if (prev_playlist->get().get_tracks_count() == 0) {
        out::debug_warn("player::play: empty playlist");
        return;
      }

      play_prev = db::track_info{
        .collection_id = playing.collection_id,
        .playlist_id = prev_playlist_id.value(),
        .track_id = prev_playlist->get().get_track_ids()[prev_playlist->get().get_tracks_count() - 1],
      };
    } else {
      seek_ms(get_current_time_ms());
      seek_ms(0);
      return;
    }
  } else if (repeat_mode == RepeatMode::ALBUM) {
    if (playlist.get_tracks_count() == 0) {
      out::debug_warn("player::play: empty playlist");
      return;
    }
    play_prev = db::track_info{
      .collection_id = playing.collection_id,
      .playlist_id = playing.playlist_id,
      .track_id = playlist.get_track_ids()[playlist.get_tracks_count() - 1],
    };
  } else {
    out::debug_warn("player::play: unknown RepeatMode enum value");
    return;
  }

  if (play_prev.has_value()) {
    playing_queue.emplace(playing_queue.begin(), play_prev.value());
    signal_on_queue_changed.emit(false);
    if (!play_track()) {
      prev_track(tries - 1);
      return;
    }
  }
}

i32 player::get_current_time_ms() {
  float ret;
  auto result = ma_sound_get_cursor_in_seconds(&sound, &ret);
  if (result != MA_SUCCESS) { return 0; }
  return ret * 1000.0f;
}

i32 player::get_total_duration_ms() { return total_duration_ms; }

bool player::is_playing() { return ma_sound_is_playing(&sound); }

bool player::is_at_end() { return ma_sound_at_end(&sound); }

std::optional<db::track_info> player::get_playing() {
  if (!playing_index.has_value()) { return std::nullopt; }
  return playing_queue[playing_index.value()];
}

const std::vector<db::track_info>& player::get_playing_queue() { return playing_queue; }
std::optional<size_t> player::get_playing_index() { return playing_index; }

void player::set_playing_index(std::optional<size_t> i) {
  if (i >= playing_queue.size()) { return; }
  playing_index = i;
  play_track();
}

player::ShuffleMode player::get_shuffle_mode() { return shuffle_mode; }

void player::set_shuffle_mode(ShuffleMode s) {
  if (s == shuffle_mode) { return; }
  shuffle_mode = s;
  mpris::notify_shuffle(s == ShuffleMode::ON);
}

player::RepeatMode player::get_repeat_mode() { return repeat_mode; }

void player::set_repeat_mode(RepeatMode r) {
  if (r == repeat_mode) { return; }
  repeat_mode = r;
  mpris::notify_loop_status(static_cast<i32>(r));
}

jt::Json player::to_json() {
  auto st = ScopeTimer("player::to_json");
  jt::Json json;
  json.setObject();
  json["repeat_mode"] = (i32)player::get_repeat_mode();
  json["shuffle_mode"] = (i32)player::get_shuffle_mode();
  json["volume"] = player::get_volume();
  json["timestamp"] = player::get_current_time_ms();
  if (player::get_playing_index().has_value()) {
    json["queue_index"] = player::get_playing_index().value_or(-1);
  } else {
    json["queue_index"] = -1;
  }
  json["queue"].setArray();
  auto& queue = json["queue"].getArray();
  for (auto& elem : get_playing_queue()) {
    auto track = db::track_by_id(elem.track_id);
    auto playlist = db::playlist_by_id(elem.playlist_id);
    auto collection = db::collection_by_id(elem.collection_id);
    if (!collection.has_value() || !playlist.has_value() || !track.has_value()) { continue; }
    queue.emplace_back(track->get().to_json());
    jt::Json& json_track = queue.back();
    json_track["playlist"] = utf32_to_utf8(playlist->get().name);
    json_track["collection"] = utf32_to_utf8(collection->get().name());
  }
  return json;
}

void player::from_json(const jt::Json& json) {
  auto st = ScopeTimer("player::from_json");
  if (!json.isObject()) { return; }
  i32 rep = (json.contains("repeat_mode") && json["repeat_mode"].isNumber()) ? json["repeat_mode"].getNumber() : 0;
  rep = std::clamp(rep, 0, (i32)player::RepeatMode::REPEAT_MODE_SIZE - 1);
  player::set_repeat_mode((player::RepeatMode)rep);
  i32 shuf = (json.contains("shuffle_mode") && json["shuffle_mode"].isNumber()) ? json["shuffle_mode"].getNumber() : 0;
  shuf = std::clamp(shuf, 0, (i32)player::ShuffleMode::SHUFFLE_MODE_SIZE - 1);
  player::set_shuffle_mode((player::ShuffleMode)shuf);
  float vol = (json.contains("volume") && json["volume"].isNumber()) ? json["volume"].getNumber() : 0;
  player::set_volume(vol);

  std::optional<size_t> queue_pos = (json.contains("queue_index") && json["queue_index"].isNumber())
                                      ? std::make_optional(json["queue_index"].getNumber())
                                      : std::nullopt;

  if (json.contains("queue") && json["queue"].isArray()) {
    auto& queue = json["queue"].getArray();
    for (auto& json_track : queue) {
      if (!json_track.isObject()) { continue; }
      auto track = db::find_track_from_json(json_track);
      if (!track.has_value()) {
        if (queue_pos.has_value()) {
          queue_pos = (queue_pos > 0 && queue_pos < player::get_playing_queue().size())
                        ? std::make_optional(queue_pos.value() - 1)
                        : std::nullopt;
        }
        continue;
      }
      player::enqueue(
        {.collection_id = track->collection_id, .playlist_id = track->playlist_id, .track_id = track->track_id},
        playing_queue.size());
    }

    player::set_playing_index(queue_pos);
    i32 prog = (json.contains("timestamp") && json["timestamp"].isNumber()) ? json["timestamp"].getNumber() : 0;
    player::seek_ms(prog);
    player::pause();
    player::signal_on_queue_changed.emit(false);
    player::signal_on_track_changed.emit();
  }
}
