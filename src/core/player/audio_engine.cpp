#include "core/player/audio_engine.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numbers>
#include <stop_token>
#include <thread>
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

    ma_result res = ma_device_init(nullptr, &config, &m_device);
    if (res != MA_SUCCESS) {
      out::critical("couldn't initialize player, error {}", static_cast<i32>(res));
      return false;
    }

    m_sample_rate = m_device.sampleRate > 0 ? m_device.sampleRate : 48000;
    m_eq.init(m_sample_rate);
    m_device_initialized = true;

    res = ma_device_start(&m_device);
    if (res != MA_SUCCESS) {
      out::critical("couldn't start audio device, error {}", static_cast<i32>(res));
      ma_device_uninit(&m_device);
      m_device_initialized = false;
      return false;
    }

    m_worker_thread = std::jthread([this](std::stop_token st) { worker_loop(st); });

    return true;
  }

  void AudioEngine::deinit() {
    if (m_worker_thread.joinable()) {
      push_command(CmdQuit{});
      m_worker_thread.request_stop();
      m_worker_thread.join();
    }

    if (m_device_initialized) {
      ma_device_uninit(&m_device);
      m_device_initialized = false;
    }

    if (m_primary.decoder != nullptr) {
      ma_decoder_uninit(m_primary.decoder.get());
      m_primary.decoder.reset();
    }
    if (m_outgoing.decoder != nullptr) {
      ma_decoder_uninit(m_outgoing.decoder.get());
      m_outgoing.decoder.reset();
    }
    if (m_incoming.decoder != nullptr) {
      ma_decoder_uninit(m_incoming.decoder.get());
      m_incoming.decoder.reset();
    }
  }

  void AudioEngine::push_command(AudioCommand cmd) {
    {
      std::lock_guard lock(m_cmd_mutex);
      m_cmd_queue.push_back(std::move(cmd));
    }
    m_cmd_cv.notify_one();
  }

  void AudioEngine::set_user_playing(bool playing) { m_user_playing.store(playing, std::memory_order_relaxed); }

  std::optional<AudioEvent> AudioEngine::pop_event() {
    std::lock_guard lock(m_evt_mutex);
    if (m_evt_queue.empty()) { return std::nullopt; }
    AudioEvent evt = std::move(m_evt_queue.front());
    m_evt_queue.pop_front();
    return evt;
  }

  void AudioEngine::push_event(AudioEvent evt) {
    std::lock_guard lock(m_evt_mutex);
    m_evt_queue.push_back(std::move(evt));
  }

  i32 AudioEngine::get_current_time_ms() const { return m_current_time_ms.load(std::memory_order_relaxed); }

  i32 AudioEngine::get_total_duration_ms() const { return m_total_duration_ms.load(std::memory_order_relaxed); }

  bool AudioEngine::get_is_seeking() const { return m_is_seeking.load(std::memory_order::relaxed); }

  bool AudioEngine::is_playing() const { return m_user_playing.load(std::memory_order_relaxed); }

  bool AudioEngine::is_at_end() const { return m_is_at_end.load(std::memory_order_relaxed); }

  void AudioEngine::process_audio(float* output, u32 frame_count) {
    size_t sample_count = static_cast<size_t>(frame_count) * 2;
    std::fill_n(output, sample_count, 0.0f);

    std::lock_guard lock(m_audio_mutex);
    if (!m_primary.active || m_primary.decoder == nullptr) { return; }

    i32 xfade_ms = m_crossfade_ms.load(std::memory_order_relaxed);
    if (xfade_ms <= 0) { m_pause_gain = m_pause_state ? 0.0f : 1.0f; }

    if (m_pause_state && m_pause_gain <= 0.0f) { return; }

    if (m_mix_buffer_a.size() < sample_count) { m_mix_buffer_a.resize(sample_count, 0.0f); }

    ma_uint64 frames_read_a = 0;
    ma_decoder_read_pcm_frames(m_primary.decoder.get(), m_mix_buffer_a.data(), frame_count, &frames_read_a);
    if (frames_read_a < frame_count) {
      std::fill(m_mix_buffer_a.begin() + frames_read_a * 2, m_mix_buffer_a.begin() + sample_count, 0.0f);
      m_primary.reached_eof = true;
    }
    m_primary.cursor_frames.fetch_add(frames_read_a, std::memory_order_relaxed);

    if (m_manual_xfade_active && m_outgoing.active && m_outgoing.decoder != nullptr) {
      if (m_mix_buffer_b.size() < sample_count) { m_mix_buffer_b.resize(sample_count, 0.0f); }

      ma_uint64 frames_read_b = 0;
      ma_decoder_read_pcm_frames(m_outgoing.decoder.get(), m_mix_buffer_b.data(), frame_count, &frames_read_b);
      if (frames_read_b < frame_count) {
        std::fill(m_mix_buffer_b.begin() + frames_read_b * 2, m_mix_buffer_b.begin() + sample_count, 0.0f);
        m_outgoing.reached_eof = true;
      }
      m_outgoing.cursor_frames.fetch_add(frames_read_b, std::memory_order_relaxed);

      for (u32 f = 0; f < frame_count; ++f) {
        float t = m_manual_xfade_total_frames > 0 ? std::clamp(static_cast<float>(m_manual_xfade_current_frame) /
                                                                 static_cast<float>(m_manual_xfade_total_frames),
                                                               0.0f, 1.0f)
                                                  : 1.0f;

        float gain_in = std::sin(t * 0.5f * std::numbers::pi_v<float>);
        float gain_out = m_outgoing.start_gain * std::cos(t * 0.5f * std::numbers::pi_v<float>);

        m_primary.current_gain = gain_in;
        m_outgoing.current_gain = gain_out;

        size_t idx = static_cast<size_t>(f) * 2;
        output[idx] = (m_mix_buffer_a[idx] * gain_in) + (m_mix_buffer_b[idx] * gain_out);
        output[idx + 1] = (m_mix_buffer_a[idx + 1] * gain_in) + (m_mix_buffer_b[idx + 1] * gain_out);

        if (m_manual_xfade_current_frame < m_manual_xfade_total_frames) { ++m_manual_xfade_current_frame; }
      }

      if (m_manual_xfade_current_frame >= m_manual_xfade_total_frames || m_outgoing.reached_eof) {
        m_outgoing.active = false;
        m_manual_xfade_active = false;
        m_primary.current_gain = 1.0f;
      }
    } else if (m_auto_xfade_active && m_incoming.active && m_incoming.decoder != nullptr) {
      if (m_mix_buffer_b.size() < sample_count) { m_mix_buffer_b.resize(sample_count, 0.0f); }

      ma_uint64 frames_read_b = 0;
      ma_decoder_read_pcm_frames(m_incoming.decoder.get(), m_mix_buffer_b.data(), frame_count, &frames_read_b);
      if (frames_read_b < frame_count) {
        std::fill(m_mix_buffer_b.begin() + frames_read_b * 2, m_mix_buffer_b.begin() + sample_count, 0.0f);
        m_incoming.reached_eof = true;
      }
      m_incoming.cursor_frames.fetch_add(frames_read_b, std::memory_order_relaxed);

      for (u32 f = 0; f < frame_count; ++f) {
        float t =
          m_auto_xfade_total_frames > 0
            ? std::clamp(static_cast<float>(m_auto_xfade_current_frame) / static_cast<float>(m_auto_xfade_total_frames),
                         0.0f, 1.0f)
            : 1.0f;

        float gain_out = m_primary.start_gain * std::cos(t * 0.5f * std::numbers::pi_v<float>);
        float gain_in = std::sin(t * 0.5f * std::numbers::pi_v<float>);

        m_primary.current_gain = gain_out;
        m_incoming.current_gain = gain_in;

        size_t idx = static_cast<size_t>(f) * 2;
        output[idx] = (m_mix_buffer_a[idx] * gain_out) + (m_mix_buffer_b[idx] * gain_in);
        output[idx + 1] = (m_mix_buffer_b[idx + 1] * gain_in) + (m_mix_buffer_a[idx + 1] * gain_out);

        if (m_auto_xfade_current_frame < m_auto_xfade_total_frames) { ++m_auto_xfade_current_frame; }
      }

      if (m_auto_xfade_current_frame >= m_auto_xfade_total_frames || m_primary.reached_eof) {
        if (m_outgoing.decoder == nullptr) {
          m_outgoing = std::move(m_primary);
          m_outgoing.active = false;
        }
        m_primary = std::move(m_incoming);
        m_primary.current_gain = 1.0f;
        m_primary.start_gain = 1.0f;
        m_incoming.active = false;
        m_auto_xfade_active = false;
        m_crossfade_triggered_for_current = false;

        i32 new_dur = static_cast<i32>((m_primary.total_frames * 1000) / m_sample_rate);
        m_total_duration_ms.store(new_dur, std::memory_order_relaxed);
        push_event(EvtAutoCrossfadePromoted{
          .track_id = m_primary.track_id,
          .duration_ms = new_dur,
        });
      }
    } else {
      std::copy_n(m_mix_buffer_a.data(), sample_count, output);
      m_primary.current_gain = 1.0f;
    }

    m_eq.process_stereo(std::span<float>(output, sample_count));

    float vol = m_volume.load(std::memory_order_relaxed);
    float pause_step =
      xfade_ms > 0 ? 1.0f / static_cast<float>(std::max<u64>(1, (static_cast<u64>(xfade_ms) * m_sample_rate) / 1000))
                   : 1.0f;

    for (u32 f = 0; f < frame_count; ++f) {
      if (m_pause_state) {
        m_pause_gain = std::max(0.0f, m_pause_gain - pause_step);
      } else {
        m_pause_gain = std::min(1.0f, m_pause_gain + pause_step);
      }
      float gain = vol * m_pause_gain;
      size_t idx = static_cast<size_t>(f) * 2;
      output[idx] *= gain;
      output[idx + 1] *= gain;
    }

    u64 cur_frames = m_primary.cursor_frames.load(std::memory_order_relaxed);
    m_current_time_ms.store(static_cast<i32>((cur_frames * 1000) / m_sample_rate), std::memory_order_relaxed);
  }

  void AudioEngine::worker_loop(std::stop_token st) {
    while (!st.stop_requested()) {
      std::deque<AudioCommand> local_cmds;
      {
        std::unique_lock lock(m_cmd_mutex);
        m_cmd_cv.wait_for(lock, std::chrono::milliseconds(100),
                          [this, &st] { return !m_cmd_queue.empty() || st.stop_requested(); });

        if (st.stop_requested()) { break; }

        local_cmds.swap(m_cmd_queue);
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
        std::lock_guard lock(m_audio_mutex);
        if (!m_manual_xfade_active && !m_outgoing.active && m_outgoing.decoder != nullptr) {
          inactive_out = std::move(m_outgoing.decoder);
        }
        if (!m_auto_xfade_active && !m_incoming.active && m_incoming.decoder != nullptr) {
          inactive_in = std::move(m_incoming.decoder);
        }

        if (!m_pause_state && m_user_playing.load(std::memory_order_relaxed) && m_primary.active &&
            m_primary.decoder != nullptr) {
          u64 cursor = m_primary.cursor_frames.load(std::memory_order_relaxed);
          u64 total = m_primary.total_frames;
          i32 xfade_ms = m_crossfade_ms.load(std::memory_order_relaxed);

          if (!m_auto_xfade_active && !m_crossfade_triggered_for_current && xfade_ms > 0 && total > 0 &&
              cursor < total) {
            i32 remaining_ms = static_cast<i32>(((total - cursor) * 1000) / m_sample_rate);
            i32 total_ms = static_cast<i32>((total * 1000) / m_sample_rate);
            if (total_ms > xfade_ms && remaining_ms <= xfade_ms) {
              m_crossfade_triggered_for_current = true;
              emit_crossfade = true;
            }
          }

          if (!m_auto_xfade_active && m_primary.reached_eof) {
            m_primary.active = false;
            m_pause_state = true;
            m_pause_gain = 0.0f;
            m_user_playing.store(false, std::memory_order_relaxed);
            m_is_at_end.store(true, std::memory_order_relaxed);
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
      ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 2, m_sample_rate);

      ma_result res{};
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

      i32 duration_ms = static_cast<i32>((total_frames * 1000) / m_sample_rate);

      {
        std::lock_guard lock(m_audio_mutex);
        i32 xfade_ms = m_crossfade_ms.load(std::memory_order_relaxed);
        bool had_audible_primary = m_primary.active && !m_primary.reached_eof && m_pause_gain > 0.01f;
        bool do_manual_xfade = play_cmd->crossfade && xfade_ms > 0 && had_audible_primary;

        if (m_auto_xfade_active && m_incoming.active) {
          if (m_incoming.current_gain > m_primary.current_gain) {
            garbage_a = std::move(m_primary.decoder);
            m_primary = std::move(m_incoming);
          } else {
            garbage_a = std::move(m_incoming.decoder);
          }
          m_incoming.active = false;
          m_auto_xfade_active = false;
        } else {
          garbage_a = std::move(m_incoming.decoder);
          m_incoming.active = false;
          m_auto_xfade_active = false;
        }

        if (do_manual_xfade) {
          garbage_b = std::move(m_outgoing.decoder);
          m_outgoing = std::move(m_primary);
          m_outgoing.start_gain = m_outgoing.current_gain * m_pause_gain;
          m_outgoing.active = true;

          m_primary.decoder = std::move(new_decoder);
          m_primary.track_id = play_cmd->track_id;
          m_primary.total_frames = total_frames;
          m_primary.cursor_frames.store(0, std::memory_order_relaxed);
          m_primary.current_gain = 0.0f;
          m_primary.start_gain = 1.0f;
          m_primary.active = true;
          m_primary.reached_eof = false;

          m_manual_xfade_active = true;
          m_manual_xfade_total_frames = std::max<u64>(1, (static_cast<u64>(xfade_ms) * m_sample_rate) / 1000);
          m_manual_xfade_current_frame = 0;
        } else {
          garbage_b = std::move(m_outgoing.decoder);
          m_outgoing.active = false;
          m_manual_xfade_active = false;

          if (m_primary.decoder != nullptr) {
            if (garbage_a == nullptr) {
              garbage_a = std::move(m_primary.decoder);
            } else {
              ma_decoder_uninit(m_primary.decoder.get());
              m_primary.decoder.reset();
            }
          }

          m_primary.decoder = std::move(new_decoder);
          m_primary.track_id = play_cmd->track_id;
          m_primary.total_frames = total_frames;
          m_primary.cursor_frames.store(0, std::memory_order_relaxed);
          m_primary.current_gain = 1.0f;
          m_primary.start_gain = 1.0f;
          m_primary.active = true;
          m_primary.reached_eof = false;

          m_eq.reset_state();
        }

        m_crossfade_triggered_for_current = false;
        m_pause_state = false;
        m_pause_gain = 1.0f;

        m_current_time_ms.store(0, std::memory_order_relaxed);
        m_total_duration_ms.store(duration_ms, std::memory_order_relaxed);
        m_is_at_end.store(false, std::memory_order_relaxed);
        m_user_playing.store(true, std::memory_order_relaxed);
      }

      push_event(EvtTrackLoaded{
        .track_id = play_cmd->track_id,
        .duration_ms = duration_ms,
      });
    } else if (const auto* auto_cmd = std::get_if<CmdStartAutoCrossfade>(&cmd)) {
      auto new_decoder = std::make_unique<ma_decoder>();
      ma_decoder_config dec_cfg = ma_decoder_config_init(ma_format_f32, 2, m_sample_rate);

      ma_result res{};
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

      std::lock_guard lock(m_audio_mutex);
      i32 xfade_ms = m_crossfade_ms.load(std::memory_order_relaxed);
      if (!m_primary.active || m_primary.reached_eof || xfade_ms <= 0) {
        garbage_a = std::move(new_decoder);
        return;
      }

      garbage_a = std::move(m_incoming.decoder);
      m_incoming.decoder = std::move(new_decoder);
      m_incoming.track_id = auto_cmd->track_id;
      m_incoming.total_frames = total_frames;
      m_incoming.cursor_frames.store(0, std::memory_order_relaxed);
      m_incoming.current_gain = 0.0f;
      m_incoming.start_gain = 1.0f;
      m_incoming.active = true;
      m_incoming.reached_eof = false;

      u64 cur = m_primary.cursor_frames.load(std::memory_order_relaxed);
      u64 rem = m_primary.total_frames > cur ? (m_primary.total_frames - cur) : 1;
      u64 requested_frames = std::max<u64>(1, (static_cast<u64>(xfade_ms) * m_sample_rate) / 1000);

      m_primary.start_gain = m_primary.current_gain;
      m_auto_xfade_total_frames = std::max<u64>(1, std::min(requested_frames, rem));
      m_auto_xfade_current_frame = 0;
      m_auto_xfade_active = true;
    } else if (std::holds_alternative<CmdCancelAutoCrossfade>(cmd)) {
      std::lock_guard lock(m_audio_mutex);
      if (m_auto_xfade_active || m_incoming.active) {
        garbage_a = std::move(m_incoming.decoder);
        m_incoming.active = false;
        m_auto_xfade_active = false;
        m_primary.current_gain = 1.0f;
        m_primary.start_gain = 1.0f;
        m_crossfade_triggered_for_current = false;
      }
    } else if (std::holds_alternative<CmdPause>(cmd)) {
      std::lock_guard lock(m_audio_mutex);
      m_pause_state = true;
      if (m_primary.cursor_frames.load(std::memory_order_relaxed) == 0) { m_pause_gain = 0.0f; }
      m_user_playing.store(false, std::memory_order_relaxed);
    } else if (std::holds_alternative<CmdResume>(cmd)) {
      std::lock_guard lock(m_audio_mutex);
      if (m_primary.active) {
        m_pause_state = false;
        m_user_playing.store(true, std::memory_order_relaxed);
      }
    } else if (std::holds_alternative<CmdStop>(cmd)) {
      std::lock_guard lock(m_audio_mutex);
      m_pause_state = true;
      m_pause_gain = 0.0f;
      m_user_playing.store(false, std::memory_order_relaxed);

      garbage_a = std::move(m_outgoing.decoder);
      m_outgoing.active = false;
      m_manual_xfade_active = false;

      garbage_b = std::move(m_incoming.decoder);
      m_incoming.active = false;
      m_auto_xfade_active = false;
      m_crossfade_triggered_for_current = false;

      if (m_primary.decoder != nullptr) {
        m_is_seeking.store(true);
        ma_decoder_seek_to_pcm_frame(m_primary.decoder.get(), 0);
        m_is_seeking.store(false);
        m_primary.cursor_frames.store(0, std::memory_order_relaxed);
        m_primary.reached_eof = false;
        m_primary.current_gain = 1.0f;
      }
      m_current_time_ms.store(0, std::memory_order_relaxed);
    } else if (const auto* seek_cmd = std::get_if<CmdSeek>(&cmd)) {
      std::lock_guard lock(m_audio_mutex);
      if (m_auto_xfade_active || m_incoming.active) {
        garbage_a = std::move(m_incoming.decoder);
        m_incoming.active = false;
        m_auto_xfade_active = false;
      }
      if (m_manual_xfade_active || m_outgoing.active) {
        garbage_b = std::move(m_outgoing.decoder);
        m_outgoing.active = false;
        m_manual_xfade_active = false;
      }

      if (m_primary.decoder != nullptr) {
        u64 target_frame = (static_cast<u64>(std::max(0, seek_cmd->position_ms)) * m_sample_rate) / 1000;
        target_frame = std::min(target_frame, m_primary.total_frames);
        m_is_seeking.store(true);
        ma_decoder_seek_to_pcm_frame(m_primary.decoder.get(), target_frame);
        m_is_seeking.store(false);
        m_primary.cursor_frames.store(target_frame, std::memory_order_relaxed);
        m_primary.reached_eof = false;
        m_primary.current_gain = 1.0f;
        m_primary.start_gain = 1.0f;
        m_crossfade_triggered_for_current = false;
        m_current_time_ms.store(seek_cmd->position_ms, std::memory_order_relaxed);
      }
    } else if (const auto* vol_cmd = std::get_if<CmdSetVolume>(&cmd)) {
      m_volume.store(std::clamp(vol_cmd->volume, 0.0f, 1.0f), std::memory_order_relaxed);
    } else if (const auto* xfade_cmd = std::get_if<CmdSetCrossfade>(&cmd)) {
      m_crossfade_ms.store(std::max(0, xfade_cmd->crossfade_ms), std::memory_order_relaxed);
    } else if (const auto* eq_cmd = std::get_if<CmdSetEqualizer>(&cmd)) {
      std::lock_guard lock(m_audio_mutex);
      m_eq.set_gains(eq_cmd->gains);
    }
  }

} // namespace player
