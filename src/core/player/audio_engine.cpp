#include "core/player/audio_engine.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include "common/logger.hpp"

namespace player {

  void AudioEngine::data_callback(ma_device* device, void* output, const void*, ma_uint32 frame_count) {
    auto* engine = static_cast<AudioEngine*>(device->pUserData);
    if (engine != nullptr) { engine->process_audio(static_cast<float*>(output), frame_count); }
  }

  bool AudioEngine::init() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = &AudioEngine::data_callback;
    config.pUserData = this;

    ma_result res = ma_device_init(nullptr, &config, &device_);
    if (res != MA_SUCCESS) {
      out::critical("couldn't initialize player, error {}", static_cast<i32>(res));
      return false;
    }

    sample_rate_ = device_.sampleRate > 0 ? device_.sampleRate : 48000;
    eq_.init(sample_rate_);
    device_initialized_ = true;

    res = ma_device_start(&device_);
    if (res != MA_SUCCESS) {
      out::critical("couldn't start audio device, error {}", static_cast<i32>(res));
      ma_device_uninit(&device_);
      device_initialized_ = false;
      return false;
    }

    worker_thread_ = std::jthread([this](std::stop_token st) { worker_loop(st); });

    return true;
  }

  void AudioEngine::deinit() {
    if (worker_thread_.joinable()) {
      push_command(CmdQuit{});
      worker_thread_.request_stop();
      worker_thread_.join();
    }

    if (device_initialized_) {
      ma_device_uninit(&device_);
      device_initialized_ = false;
    }

    if (primary_.decoder != nullptr) {
      ma_decoder_uninit(primary_.decoder.get());
      primary_.decoder.reset();
    }
    if (outgoing_.decoder != nullptr) {
      ma_decoder_uninit(outgoing_.decoder.get());
      outgoing_.decoder.reset();
    }
    if (incoming_.decoder != nullptr) {
      ma_decoder_uninit(incoming_.decoder.get());
      incoming_.decoder.reset();
    }
  }

  void AudioEngine::push_command(AudioCommand cmd) {
    {
      std::lock_guard lock(cmd_mutex_);
      cmd_queue_.push_back(std::move(cmd));
    }
    cmd_cv_.notify_one();
  }

  void AudioEngine::set_user_playing(bool playing) { user_playing_.store(playing, std::memory_order_relaxed); }

  std::optional<AudioEvent> AudioEngine::pop_event() {
    std::lock_guard lock(evt_mutex_);
    if (evt_queue_.empty()) { return std::nullopt; }
    AudioEvent evt = std::move(evt_queue_.front());
    evt_queue_.pop_front();
    return evt;
  }

  void AudioEngine::push_event(AudioEvent evt) {
    std::lock_guard lock(evt_mutex_);
    evt_queue_.push_back(std::move(evt));
  }

  i32 AudioEngine::get_current_time_ms() const { return current_time_ms_.load(std::memory_order_relaxed); }

  i32 AudioEngine::get_total_duration_ms() const { return total_duration_ms_.load(std::memory_order_relaxed); }

  bool AudioEngine::is_playing() const { return user_playing_.load(std::memory_order_relaxed); }

  bool AudioEngine::is_at_end() const { return is_at_end_.load(std::memory_order_relaxed); }

  void AudioEngine::process_audio(float* output, u32 frame_count) {
    size_t sample_count = static_cast<size_t>(frame_count) * 2;
    std::fill_n(output, sample_count, 0.0f);

    std::lock_guard lock(audio_mutex_);
    if (!primary_.active || primary_.decoder == nullptr) { return; }

    i32 xfade_ms = crossfade_ms_.load(std::memory_order_relaxed);
    if (xfade_ms <= 0) { pause_gain_ = pause_state_ ? 0.0f : 1.0f; }

    if (pause_state_ && pause_gain_ <= 0.0f) { return; }

    if (mix_buffer_a_.size() < sample_count) { mix_buffer_a_.resize(sample_count, 0.0f); }

    ma_uint64 frames_read_a = 0;
    ma_decoder_read_pcm_frames(primary_.decoder.get(), mix_buffer_a_.data(), frame_count, &frames_read_a);
    if (frames_read_a < frame_count) {
      std::fill(mix_buffer_a_.begin() + frames_read_a * 2, mix_buffer_a_.begin() + sample_count, 0.0f);
      primary_.reached_eof = true;
    }
    primary_.cursor_frames.fetch_add(frames_read_a, std::memory_order_relaxed);

    if (manual_xfade_active_ && outgoing_.active && outgoing_.decoder != nullptr) {
      if (mix_buffer_b_.size() < sample_count) { mix_buffer_b_.resize(sample_count, 0.0f); }

      ma_uint64 frames_read_b = 0;
      ma_decoder_read_pcm_frames(outgoing_.decoder.get(), mix_buffer_b_.data(), frame_count, &frames_read_b);
      if (frames_read_b < frame_count) {
        std::fill(mix_buffer_b_.begin() + frames_read_b * 2, mix_buffer_b_.begin() + sample_count, 0.0f);
        outgoing_.reached_eof = true;
      }
      outgoing_.cursor_frames.fetch_add(frames_read_b, std::memory_order_relaxed);

      for (u32 f = 0; f < frame_count; ++f) {
        float t = manual_xfade_total_frames_ > 0 ? std::clamp(static_cast<float>(manual_xfade_current_frame_) /
                                                                static_cast<float>(manual_xfade_total_frames_),
                                                              0.0f, 1.0f)
                                                 : 1.0f;

        float gain_in = std::sin(t * 0.5f * std::numbers::pi_v<float>);
        float gain_out = outgoing_.start_gain * std::cos(t * 0.5f * std::numbers::pi_v<float>);

        primary_.current_gain = gain_in;
        outgoing_.current_gain = gain_out;

        size_t idx = static_cast<size_t>(f) * 2;
        output[idx] = mix_buffer_a_[idx] * gain_in + mix_buffer_b_[idx] * gain_out;
        output[idx + 1] = mix_buffer_a_[idx + 1] * gain_in + mix_buffer_b_[idx + 1] * gain_out;

        if (manual_xfade_current_frame_ < manual_xfade_total_frames_) { ++manual_xfade_current_frame_; }
      }

      if (manual_xfade_current_frame_ >= manual_xfade_total_frames_ || outgoing_.reached_eof) {
        outgoing_.active = false;
        manual_xfade_active_ = false;
        primary_.current_gain = 1.0f;
      }
    } else if (auto_xfade_active_ && incoming_.active && incoming_.decoder != nullptr) {
      if (mix_buffer_b_.size() < sample_count) { mix_buffer_b_.resize(sample_count, 0.0f); }

      ma_uint64 frames_read_b = 0;
      ma_decoder_read_pcm_frames(incoming_.decoder.get(), mix_buffer_b_.data(), frame_count, &frames_read_b);
      if (frames_read_b < frame_count) {
        std::fill(mix_buffer_b_.begin() + frames_read_b * 2, mix_buffer_b_.begin() + sample_count, 0.0f);
        incoming_.reached_eof = true;
      }
      incoming_.cursor_frames.fetch_add(frames_read_b, std::memory_order_relaxed);

      for (u32 f = 0; f < frame_count; ++f) {
        float t =
          auto_xfade_total_frames_ > 0
            ? std::clamp(static_cast<float>(auto_xfade_current_frame_) / static_cast<float>(auto_xfade_total_frames_),
                         0.0f, 1.0f)
            : 1.0f;

        float gain_out = primary_.start_gain * std::cos(t * 0.5f * std::numbers::pi_v<float>);
        float gain_in = std::sin(t * 0.5f * std::numbers::pi_v<float>);

        primary_.current_gain = gain_out;
        incoming_.current_gain = gain_in;

        size_t idx = static_cast<size_t>(f) * 2;
        output[idx] = mix_buffer_a_[idx] * gain_out + mix_buffer_b_[idx] * gain_in;
        output[idx + 1] = mix_buffer_a_[idx + 1] * gain_out + mix_buffer_b_[idx + 1] * gain_in;

        if (auto_xfade_current_frame_ < auto_xfade_total_frames_) { ++auto_xfade_current_frame_; }
      }

      if (auto_xfade_current_frame_ >= auto_xfade_total_frames_ || primary_.reached_eof) {
        if (outgoing_.decoder == nullptr) {
          outgoing_ = std::move(primary_);
          outgoing_.active = false;
        }
        primary_ = std::move(incoming_);
        primary_.current_gain = 1.0f;
        primary_.start_gain = 1.0f;
        incoming_.active = false;
        auto_xfade_active_ = false;
        crossfade_triggered_for_current_ = false;

        i32 new_dur = static_cast<i32>((primary_.total_frames * 1000) / sample_rate_);
        total_duration_ms_.store(new_dur, std::memory_order_relaxed);
        push_event(EvtAutoCrossfadePromoted{
          .track_id = primary_.track_id,
          .duration_ms = new_dur,
        });
      }
    } else {
      std::copy_n(mix_buffer_a_.data(), sample_count, output);
      primary_.current_gain = 1.0f;
    }

    eq_.process_stereo(std::span<float>(output, sample_count));

    float vol = volume_.load(std::memory_order_relaxed);
    float pause_step =
      xfade_ms > 0 ? 1.0f / static_cast<float>(std::max<u64>(1, (static_cast<u64>(xfade_ms) * sample_rate_) / 1000))
                   : 1.0f;

    for (u32 f = 0; f < frame_count; ++f) {
      if (pause_state_) {
        pause_gain_ = std::max(0.0f, pause_gain_ - pause_step);
      } else {
        pause_gain_ = std::min(1.0f, pause_gain_ + pause_step);
      }
      float gain = vol * pause_gain_;
      size_t idx = static_cast<size_t>(f) * 2;
      output[idx] *= gain;
      output[idx + 1] *= gain;
    }

    u64 cur_frames = primary_.cursor_frames.load(std::memory_order_relaxed);
    current_time_ms_.store(static_cast<i32>((cur_frames * 1000) / sample_rate_), std::memory_order_relaxed);
  }

  void AudioEngine::worker_loop(std::stop_token st) {
    while (!st.stop_requested()) {
      std::deque<AudioCommand> local_cmds;
      {
        std::unique_lock lock(cmd_mutex_);
        cmd_cv_.wait_for(lock, std::chrono::milliseconds(15),
                         [this, &st] { return !cmd_queue_.empty() || st.stop_requested(); });

        if (st.stop_requested()) { break; }

        local_cmds.swap(cmd_queue_);
      }

      for (const auto& cmd : local_cmds) {
        if (std::holds_alternative<CmdQuit>(cmd)) { return; }
        std::unique_ptr<ma_decoder> garbage_a{};
        std::unique_ptr<ma_decoder> garbage_b{};
        handle_command(cmd, garbage_a, garbage_b);
        if (garbage_a != nullptr) { ma_decoder_uninit(garbage_a.get()); }
        if (garbage_b != nullptr) { ma_decoder_uninit(garbage_b.get()); }
      }

      std::unique_ptr<ma_decoder> inactive_out{};
      std::unique_ptr<ma_decoder> inactive_in{};
      bool emit_crossfade = false;
      bool emit_ended = false;

      {
        std::lock_guard lock(audio_mutex_);
        if (!manual_xfade_active_ && !outgoing_.active && outgoing_.decoder != nullptr) {
          inactive_out = std::move(outgoing_.decoder);
        }
        if (!auto_xfade_active_ && !incoming_.active && incoming_.decoder != nullptr) {
          inactive_in = std::move(incoming_.decoder);
        }

        if (!pause_state_ && user_playing_.load(std::memory_order_relaxed) && primary_.active &&
            primary_.decoder != nullptr) {
          u64 cursor = primary_.cursor_frames.load(std::memory_order_relaxed);
          u64 total = primary_.total_frames;
          i32 xfade_ms = crossfade_ms_.load(std::memory_order_relaxed);

          if (!auto_xfade_active_ && !crossfade_triggered_for_current_ && xfade_ms > 0 && total > 0 && cursor < total) {
            i32 remaining_ms = static_cast<i32>(((total - cursor) * 1000) / sample_rate_);
            i32 total_ms = static_cast<i32>((total * 1000) / sample_rate_);
            if (total_ms > xfade_ms && remaining_ms <= xfade_ms) {
              crossfade_triggered_for_current_ = true;
              emit_crossfade = true;
            }
          }

          if (!auto_xfade_active_ && primary_.reached_eof) {
            primary_.active = false;
            pause_state_ = true;
            pause_gain_ = 0.0f;
            user_playing_.store(false, std::memory_order_relaxed);
            is_at_end_.store(true, std::memory_order_relaxed);
            emit_ended = true;
          }
        }
      }

      if (inactive_out != nullptr) { ma_decoder_uninit(inactive_out.get()); }
      if (inactive_in != nullptr) { ma_decoder_uninit(inactive_in.get()); }
      if (emit_crossfade) { push_event(EvtCrossfadeTrigger{}); }
      if (emit_ended) { push_event(EvtTrackEnded{}); }
    }
  }

  void AudioEngine::handle_command(const AudioCommand& cmd, std::unique_ptr<ma_decoder>& garbage_a,
                                   std::unique_ptr<ma_decoder>& garbage_b) {
    if (const auto* play_cmd = std::get_if<CmdPlay>(&cmd)) {
      auto new_decoder = std::make_unique<ma_decoder>();
      ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 2, sample_rate_);

      ma_result res;
#if defined(_WIN32)
      res = ma_decoder_init_file_w(play_cmd->file_path.c_str(), &dec_cfg, new_decoder.get());
#else
      res = ma_decoder_init_file(play_cmd->file_path.c_str(), &dec_cfg, new_decoder.get());
#endif

      if (res != MA_SUCCESS) {
        push_event(EvtTrackError{
          .track_id = play_cmd->track_id,
          .file_path = play_cmd->file_path,
          .error_code = static_cast<i32>(res),
          .was_auto_crossfade = false,
        });
        return;
      }

      ma_uint64 total_frames = 0;
      res = ma_decoder_get_length_in_pcm_frames(new_decoder.get(), &total_frames);
      if (res != MA_SUCCESS) {
        ma_decoder_uninit(new_decoder.get());
        push_event(EvtTrackError{
          .track_id = play_cmd->track_id,
          .file_path = play_cmd->file_path,
          .error_code = static_cast<i32>(res),
          .was_auto_crossfade = false,
        });
        return;
      }

      i32 duration_ms = static_cast<i32>((total_frames * 1000) / sample_rate_);

      {
        std::lock_guard lock(audio_mutex_);
        i32 xfade_ms = crossfade_ms_.load(std::memory_order_relaxed);
        bool had_audible_primary = primary_.active && !primary_.reached_eof && pause_gain_ > 0.01f;
        bool do_manual_xfade = play_cmd->crossfade && xfade_ms > 0 && had_audible_primary;

        if (auto_xfade_active_ && incoming_.active) {
          if (incoming_.current_gain > primary_.current_gain) {
            garbage_a = std::move(primary_.decoder);
            primary_ = std::move(incoming_);
          } else {
            garbage_a = std::move(incoming_.decoder);
          }
          incoming_.active = false;
          auto_xfade_active_ = false;
        } else {
          garbage_a = std::move(incoming_.decoder);
          incoming_.active = false;
          auto_xfade_active_ = false;
        }

        if (do_manual_xfade) {
          garbage_b = std::move(outgoing_.decoder);
          outgoing_ = std::move(primary_);
          outgoing_.start_gain = outgoing_.current_gain * pause_gain_;
          outgoing_.active = true;

          primary_.decoder = std::move(new_decoder);
          primary_.track_id = play_cmd->track_id;
          primary_.total_frames = total_frames;
          primary_.cursor_frames.store(0, std::memory_order_relaxed);
          primary_.current_gain = 0.0f;
          primary_.start_gain = 1.0f;
          primary_.active = true;
          primary_.reached_eof = false;

          manual_xfade_active_ = true;
          manual_xfade_total_frames_ = std::max<u64>(1, (static_cast<u64>(xfade_ms) * sample_rate_) / 1000);
          manual_xfade_current_frame_ = 0;
        } else {
          garbage_b = std::move(outgoing_.decoder);
          outgoing_.active = false;
          manual_xfade_active_ = false;

          if (primary_.decoder != nullptr) {
            if (garbage_a == nullptr) {
              garbage_a = std::move(primary_.decoder);
            } else {
              ma_decoder_uninit(primary_.decoder.get());
              primary_.decoder.reset();
            }
          }

          primary_.decoder = std::move(new_decoder);
          primary_.track_id = play_cmd->track_id;
          primary_.total_frames = total_frames;
          primary_.cursor_frames.store(0, std::memory_order_relaxed);
          primary_.current_gain = 1.0f;
          primary_.start_gain = 1.0f;
          primary_.active = true;
          primary_.reached_eof = false;

          eq_.reset_state();
        }

        crossfade_triggered_for_current_ = false;
        pause_state_ = false;
        pause_gain_ = 1.0f;

        current_time_ms_.store(0, std::memory_order_relaxed);
        total_duration_ms_.store(duration_ms, std::memory_order_relaxed);
        is_at_end_.store(false, std::memory_order_relaxed);
        user_playing_.store(true, std::memory_order_relaxed);
      }

      push_event(EvtTrackLoaded{
        .track_id = play_cmd->track_id,
        .duration_ms = duration_ms,
      });
    } else if (const auto* auto_cmd = std::get_if<CmdStartAutoCrossfade>(&cmd)) {
      auto new_decoder = std::make_unique<ma_decoder>();
      ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 2, sample_rate_);

      ma_result res;
#if defined(_WIN32)
      res = ma_decoder_init_file_w(auto_cmd->file_path.c_str(), &dec_cfg, new_decoder.get());
#else
      res = ma_decoder_init_file(auto_cmd->file_path.c_str(), &dec_cfg, new_decoder.get());
#endif

      if (res != MA_SUCCESS) {
        push_event(EvtTrackError{
          .track_id = auto_cmd->track_id,
          .file_path = auto_cmd->file_path,
          .error_code = static_cast<i32>(res),
          .was_auto_crossfade = true,
        });
        return;
      }

      ma_uint64 total_frames = 0;
      res = ma_decoder_get_length_in_pcm_frames(new_decoder.get(), &total_frames);
      if (res != MA_SUCCESS) {
        ma_decoder_uninit(new_decoder.get());
        push_event(EvtTrackError{
          .track_id = auto_cmd->track_id,
          .file_path = auto_cmd->file_path,
          .error_code = static_cast<i32>(res),
          .was_auto_crossfade = true,
        });
        return;
      }

      std::lock_guard lock(audio_mutex_);
      i32 xfade_ms = crossfade_ms_.load(std::memory_order_relaxed);
      if (!primary_.active || primary_.reached_eof || xfade_ms <= 0) {
        garbage_a = std::move(new_decoder);
        return;
      }

      garbage_a = std::move(incoming_.decoder);
      incoming_.decoder = std::move(new_decoder);
      incoming_.track_id = auto_cmd->track_id;
      incoming_.total_frames = total_frames;
      incoming_.cursor_frames.store(0, std::memory_order_relaxed);
      incoming_.current_gain = 0.0f;
      incoming_.start_gain = 1.0f;
      incoming_.active = true;
      incoming_.reached_eof = false;

      u64 cur = primary_.cursor_frames.load(std::memory_order_relaxed);
      u64 rem = primary_.total_frames > cur ? (primary_.total_frames - cur) : 1;
      u64 requested_frames = std::max<u64>(1, (static_cast<u64>(xfade_ms) * sample_rate_) / 1000);

      primary_.start_gain = primary_.current_gain;
      auto_xfade_total_frames_ = std::max<u64>(1, std::min(requested_frames, rem));
      auto_xfade_current_frame_ = 0;
      auto_xfade_active_ = true;
    } else if (std::holds_alternative<CmdCancelAutoCrossfade>(cmd)) {
      std::lock_guard lock(audio_mutex_);
      if (auto_xfade_active_ || incoming_.active) {
        garbage_a = std::move(incoming_.decoder);
        incoming_.active = false;
        auto_xfade_active_ = false;
        primary_.current_gain = 1.0f;
        primary_.start_gain = 1.0f;
        crossfade_triggered_for_current_ = false;
      }
    } else if (std::holds_alternative<CmdPause>(cmd)) {
      std::lock_guard lock(audio_mutex_);
      pause_state_ = true;
      if (primary_.cursor_frames.load(std::memory_order_relaxed) == 0) { pause_gain_ = 0.0f; }
      user_playing_.store(false, std::memory_order_relaxed);
    } else if (std::holds_alternative<CmdResume>(cmd)) {
      std::lock_guard lock(audio_mutex_);
      if (primary_.active) {
        pause_state_ = false;
        user_playing_.store(true, std::memory_order_relaxed);
      }
    } else if (std::holds_alternative<CmdStop>(cmd)) {
      std::lock_guard lock(audio_mutex_);
      pause_state_ = true;
      pause_gain_ = 0.0f;
      user_playing_.store(false, std::memory_order_relaxed);

      garbage_a = std::move(outgoing_.decoder);
      outgoing_.active = false;
      manual_xfade_active_ = false;

      garbage_b = std::move(incoming_.decoder);
      incoming_.active = false;
      auto_xfade_active_ = false;
      crossfade_triggered_for_current_ = false;

      if (primary_.decoder != nullptr) {
        ma_decoder_seek_to_pcm_frame(primary_.decoder.get(), 0);
        primary_.cursor_frames.store(0, std::memory_order_relaxed);
        primary_.reached_eof = false;
        primary_.current_gain = 1.0f;
      }
      current_time_ms_.store(0, std::memory_order_relaxed);
    } else if (const auto* seek_cmd = std::get_if<CmdSeek>(&cmd)) {
      std::lock_guard lock(audio_mutex_);
      if (auto_xfade_active_ || incoming_.active) {
        garbage_a = std::move(incoming_.decoder);
        incoming_.active = false;
        auto_xfade_active_ = false;
      }
      if (manual_xfade_active_ || outgoing_.active) {
        garbage_b = std::move(outgoing_.decoder);
        outgoing_.active = false;
        manual_xfade_active_ = false;
      }

      if (primary_.decoder != nullptr) {
        u64 target_frame = (static_cast<u64>(std::max(0, seek_cmd->position_ms)) * sample_rate_) / 1000;
        target_frame = std::min(target_frame, primary_.total_frames);
        ma_decoder_seek_to_pcm_frame(primary_.decoder.get(), target_frame);
        primary_.cursor_frames.store(target_frame, std::memory_order_relaxed);
        primary_.reached_eof = false;
        primary_.current_gain = 1.0f;
        primary_.start_gain = 1.0f;
        crossfade_triggered_for_current_ = false;
        current_time_ms_.store(seek_cmd->position_ms, std::memory_order_relaxed);
      }
    } else if (const auto* vol_cmd = std::get_if<CmdSetVolume>(&cmd)) {
      volume_.store(std::clamp(vol_cmd->volume, 0.0f, 1.0f), std::memory_order_relaxed);
    } else if (const auto* xfade_cmd = std::get_if<CmdSetCrossfade>(&cmd)) {
      crossfade_ms_.store(std::max(0, xfade_cmd->crossfade_ms), std::memory_order_relaxed);
    } else if (const auto* eq_cmd = std::get_if<CmdSetEqualizer>(&cmd)) {
      std::lock_guard lock(audio_mutex_);
      eq_.set_gains(eq_cmd->gains);
    }
  }

} // namespace player
