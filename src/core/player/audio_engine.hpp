#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <variant>
#include <vector>
#include "common/types.hpp"
#include "core/player/equalizer.hpp"
#include "lib/miniaudio/miniaudio.h"

namespace player {

  struct CmdPlay {
      size_t track_id{};
      std::filesystem::path file_path;
      bool crossfade = false;
  };

  struct CmdStartAutoCrossfade {
      size_t track_id{};
      std::filesystem::path file_path;
  };

  struct CmdCancelAutoCrossfade {};
  struct CmdPause {};
  struct CmdResume {};
  struct CmdStop {};

  struct CmdSeek {
      i32 position_ms{};
  };

  struct CmdSetVolume {
      float volume{};
  };

  struct CmdSetCrossfade {
      i32 crossfade_ms{};
  };

  struct CmdSetEqualizer {
      EqualizerGains gains{};
  };

  struct CmdQuit {};

  using AudioCommand = std::variant<CmdPlay, CmdStartAutoCrossfade, CmdCancelAutoCrossfade, CmdPause, CmdResume,
                                    CmdStop, CmdSeek, CmdSetVolume, CmdSetCrossfade, CmdSetEqualizer, CmdQuit>;

  struct EvtTrackLoaded {
      size_t track_id{};
      i32 duration_ms{};
  };

  struct EvtTrackError {
      size_t track_id{};
      std::filesystem::path file_path;
      i32 error_code{};
      bool was_auto_crossfade = false;
  };

  struct EvtCrossfadeTrigger {};

  struct EvtAutoCrossfadePromoted {
      size_t track_id{};
      i32 duration_ms{};
  };

  struct EvtTrackEnded {};

  using AudioEvent =
    std::variant<EvtTrackLoaded, EvtTrackError, EvtCrossfadeTrigger, EvtAutoCrossfadePromoted, EvtTrackEnded>;

  class AudioEngine {
    public:
      bool init();
      void deinit();

      void push_command(AudioCommand cmd);
      void set_user_playing(bool playing);
      std::optional<AudioEvent> pop_event();

      i32 get_current_time_ms() const;
      i32 get_total_duration_ms() const;
      bool get_is_seeking() const;
      bool is_playing() const;
      bool is_at_end() const;

    private:
      struct Deck final {
          std::unique_ptr<ma_decoder> decoder;
          size_t track_id{};
          u64 total_frames{};
          std::atomic<u64> cursor_frames{0};
          float current_gain = 1.0f;
          float start_gain = 1.0f;
          bool active = false;
          bool reached_eof = false;

          Deck() = default;
          Deck(const Deck&) = delete;
          Deck& operator=(const Deck&) = delete;
          ~Deck() = default;
          Deck(Deck&& other) noexcept
            : decoder(std::move(other.decoder)), track_id(other.track_id), total_frames(other.total_frames),
              cursor_frames(other.cursor_frames.load(std::memory_order_relaxed)), current_gain(other.current_gain),
              start_gain(other.start_gain), active(other.active), reached_eof(other.reached_eof) {
            other.active = false;
          }

          Deck& operator=(Deck&& other) noexcept {
            if (this != &other) {
              decoder = std::move(other.decoder);
              track_id = other.track_id;
              total_frames = other.total_frames;
              cursor_frames.store(other.cursor_frames.load(std::memory_order_relaxed), std::memory_order_relaxed);
              current_gain = other.current_gain;
              start_gain = other.start_gain;
              active = other.active;
              reached_eof = other.reached_eof;
              other.active = false;
            }
            return *this;
          }
      };

      static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
      void process_audio(float* output, u32 frame_count);
      void worker_loop(std::stop_token st);
      void handle_command(const AudioCommand& cmd, std::unique_ptr<ma_decoder>& garbage_a,
                          std::unique_ptr<ma_decoder>& garbage_b);
      void push_event(AudioEvent evt);

      ma_device m_device{};
      bool m_device_initialized = false;
      u32 m_sample_rate = 48000;

      std::jthread m_worker_thread;

      std::mutex m_cmd_mutex;
      std::condition_variable m_cmd_cv;
      std::deque<AudioCommand> m_cmd_queue;

      std::mutex m_evt_mutex;
      std::deque<AudioEvent> m_evt_queue;

      std::mutex m_audio_mutex;
      Deck m_primary;
      Deck m_outgoing;
      Deck m_incoming;

      bool m_manual_xfade_active = false;
      u64 m_manual_xfade_total_frames = 0;
      u64 m_manual_xfade_current_frame = 0;

      bool m_auto_xfade_active = false;
      u64 m_auto_xfade_total_frames = 0;
      u64 m_auto_xfade_current_frame = 0;
      bool m_crossfade_triggered_for_current = false;

      bool m_pause_state = true;
      float m_pause_gain = 0.0f;

      Equalizer m_eq{};
      std::vector<float> m_mix_buffer_a;
      std::vector<float> m_mix_buffer_b;

      std::atomic<float> m_volume{0.5f};
      std::atomic<i32> m_crossfade_ms{0};
      std::atomic<i32> m_current_time_ms{0};
      std::atomic<i32> m_total_duration_ms{0};
      std::atomic<i32> m_is_seeking{false};
      std::atomic<bool> m_user_playing{false};
      std::atomic<bool> m_is_at_end{false};
  };

} // namespace player
