#pragma once
#include <chrono>
#include <string>
#include "common/logger.hpp"

#ifdef NDEBUG
#define ensure(condition)
#else
#define ensure(condition)                                                                                              \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      out::debug_critical("Assertion '{}' failed!", #condition);                                                       \
      exit(1);                                                                                                         \
    }                                                                                                                  \
  } while (false)
#endif

struct ScopeTimer final {
  private:
    std::string m_message;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;
    double m_ms_threshold;

  public:
    ScopeTimer(const std::string& _message = "", double ms_threshold_ = 0.5)
      : m_message(_message), m_ms_threshold(ms_threshold_) {
      m_start_time = std::chrono::high_resolution_clock::now();
    }

    ScopeTimer(const ScopeTimer&) = default;
    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = default;
    ScopeTimer& operator=(ScopeTimer&&) = delete;
    ~ScopeTimer() {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - m_start_time);

      if (microseconds.count() >= m_ms_threshold) {
        if (m_message.empty()) {
          out::debug_info("took {}ms", microseconds.count() / 1000.0);
        } else {
          out::debug_info("{} took {}ms", m_message, microseconds.count() / 1000.0);
        }
      }
    }
};
