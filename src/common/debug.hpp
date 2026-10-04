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
  } while (false);
#endif

struct ScopeTimer {
    std::string message;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    double ms_threshold;

    ScopeTimer(const std::string& _message = "", double ms_threshold_ = 0.5)
      : message(_message), ms_threshold(ms_threshold_) {
      start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

      if (microseconds.count() >= ms_threshold) {
        if (message.empty()) {
          out::debug_info("took {}ms", microseconds.count() / 1000.0);
        } else {
          out::debug_info("{} took {}ms", message, microseconds.count() / 1000.0);
        }
      }
    }
};
