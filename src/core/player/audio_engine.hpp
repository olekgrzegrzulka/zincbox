#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <variant>
#include <vector>
#include "common/types.hpp"
#include "core/player/equalizer.hpp"
#include "lib/miniaudio/miniaudio.h"

namespace player {

  struct CmdPlay {
      size_t track_id{};
      std::filesystem::path file_path{};
      bool crossfade = false;
  };

  struct CmdStartAutoCrossfade {
      size_t track_id{};
      std::filesystem::path file_path{};
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
      std::filesystem::path file_path{};
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
      bool is_playing() const;
      bool is_at_end() const;

    private:
      struct Deck {
          std::unique_ptr<ma_decoder> decoder{};
          size_t track_id{};
          u64 total_frames{};
          std::atomic<u64> cursor_frames{0};
          float current_gain = 1.0f;
          float start_gain = 1.0f;
          bool active = false;
          bool reached_eof = false;

          Deck() = default;
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

      ma_device device_{};
      bool device_initialized_ = false;
      u32 sample_rate_ = 48000;

      std::jthread worker_thread_{};

      std::mutex cmd_mutex_{};
      std::condition_variable cmd_cv_{};
      std::deque<AudioCommand> cmd_queue_{};

      std::mutex evt_mutex_{};
      std::deque<AudioEvent> evt_queue_{};

      std::mutex audio_mutex_{};
      Deck primary_{};
      Deck outgoing_{};
      Deck incoming_{};

      bool manual_xfade_active_ = false;
      u64 manual_xfade_total_frames_ = 0;
      u64 manual_xfade_current_frame_ = 0;

      bool auto_xfade_active_ = false;
      u64 auto_xfade_total_frames_ = 0;
      u64 auto_xfade_current_frame_ = 0;
      bool crossfade_triggered_for_current_ = false;

      bool pause_state_ = true;
      float pause_gain_ = 0.0f;

      Equalizer eq_{};
      std::vector<float> mix_buffer_a_{};
      std::vector<float> mix_buffer_b_{};

      std::atomic<float> volume_{0.5f};
      std::atomic<i32> crossfade_ms_{0};
      std::atomic<i32> current_time_ms_{0};
      std::atomic<i32> total_duration_ms_{0};
      std::atomic<bool> user_playing_{false};
      std::atomic<bool> is_at_end_{false};
  };

} // namespace player
