#pragma once
#include <chrono>
#include <string>
#include "common/logger.hpp"

#define ensure(condition)                                        \
  do {                                                           \
    if (!(condition)) {                                          \
      out::debug_critical("Assertion '{}' failed!", #condition); \
      exit(1);                                                   \
    }                                                            \
  } while (false);

struct ScopeTimer {
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    double ms_threshold;

    ScopeTimer(std::string _message = "", double ms_threshold_ = 0.5) : message(_message), ms_threshold(ms_threshold_) {
      start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

      if (milliseconds.count() >= ms_threshold) {
        if (message.empty()) {
          out::debug_info("took {}ms", milliseconds.count());
        } else {
          out::debug_info("{} took {}ms", message, milliseconds.count());
        }
      }
    }
};
