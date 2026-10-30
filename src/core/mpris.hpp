#pragma once
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include "common/types.hpp"

namespace mpris {
  enum class CommandType : u8 { PLAY, PAUSE, NEXT, PREVIOUS, STOP, SET, SEEK, PLAY_PAUSE, LOOP, SHUFFLE };

  enum class LoopStatus : u8 { NONE, TRACK, PLAYLIST };

  struct Command final {
      CommandType type{};
      i64 value{};
  };

  class CommandQueue final {
    private:
      std::queue<Command> m_queue;
      std::mutex m_mutex;

    public:
      void push(Command cmd) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(cmd);
      }

      std::optional<Command> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) { return std::nullopt; }
        auto ret = m_queue.front();
        m_queue.pop();
        return ret;
      }
  };

  void init();
  void deinit();
  std::optional<Command> command_pop();

  void notify_playback_status_playing();
  void notify_playback_status_paused();
  void notify_playback_status_stopped();
  void notify_track_change(std::string title, std::string artist, std::string album, i64 duration_ms,
                           std::string cover_path);
  void notify_cover_change(std::string_view cover_path);
  void notify_volume(double volume);
  void notify_seeked(i64 position_ms);
  void notify_loop_status(i32);
  void notify_shuffle(bool);
} // namespace mpris
