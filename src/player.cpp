#pragma once
#include <string>
#include "../lib/miniaudio/miniaudio.h"
#include "debug.hpp"
#include "player.hpp"

ma_engine engine{};
ma_sound sound{};
ma_device device{};

i32 total_duration_ms{};

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

void player::play() {
  ma_sound_start(&sound);
}

void player::play(const std::string& path) {
  ma_engine_stop(&engine);
  ma_sound_uninit(&sound);
  debug_log("player::play(", path, ")");

  ma_result result;

  result = ma_sound_init_from_file(&engine, path.c_str(), MA_SOUND_FLAG_NO_PITCH, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    debug_warn(result);
    return;
  }

  float ret;
  result = ma_sound_get_length_in_seconds(&sound, &ret);
  if (result != MA_SUCCESS) { total_duration_ms = 0; }
  total_duration_ms = ret * 1000.0f;

  ma_sound_start(&sound);
  ma_engine_start(&engine);
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
