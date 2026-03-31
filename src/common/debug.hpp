#pragma once
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"

#ifdef NDEBUG

template <typename T>
static void print(T) {
}

template <typename... Args>
static void print_(Args&&...) {
}

#else

template <typename T>
static void print_(T&& arg) {
  std::cout << arg;
}

template <typename T>
static void print_(glm::vec<2, T> vec2) {
  std::cout << "[" << vec2.x << ", " << vec2.y << "]";
}

template <typename T>
static void print_(glm::vec<3, T> vec3) {
  std::cout << "[" << vec3.x << ", " << vec3.y << ", " << vec3.z << "]";
}

template <typename T>
static void print_(glm::vec<4, T> vec4) {
  std::cout << "[" << vec4.x << ", " << vec4.y << ", " << vec4.z << ", " << vec4.w << "]";
}

[[maybe_unused]] static void print_(rgba c) {
  std::cout << "rgba(" << (i32)c.r << ", " << (i32)c.g << ", " << (i32)c.b << ", " << (i32)c.a << ")";
}

[[maybe_unused]] static void print_(rect2i r) {
  std::cout << "rect2i{" << r.begin.x << ", " << r.begin.y << "; " << r.size.x << ", " << r.size.y << "}";
}

template <typename T>
static void print_(std::vector<T> vec) {
  std::cout << "{";
  for (size_t i = 0; i < vec.size(); i += 1) {
    print_(vec[i]);
    if (i != vec.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "}";
}

template <typename T>
static void print(T arg) {
  print_(arg);
  std::cout << std::endl;
}

#endif

template <typename T, typename... R>
static void print(T&& first_arg, R&&... args) {
  print_(first_arg);
  print(args...);
}

#define __FILENAME__ strrchr("/" __FILE__, '/') + 1

#define debug_log(...) \
  print("\033[1;36m", "[LOG] \033[1;37m", __FILENAME__, ":", __LINE__, " ", "\033[0m", __VA_ARGS__)

#define debug_log_no_filename(...) \
  print("\033[1;36m", "[LOG]\033[0m ", __VA_ARGS__)

#define debug_warn(...) \
  print("\033[1;33m", "[WARN] \033[1;37m", __FILENAME__, ":", __LINE__, " ", "\033[0m", __VA_ARGS__)

#define debug_error(...)                                                                               \
  print("\033[1;31m", "[ERROR] \033[1;37m", __FILENAME__, ":", __LINE__, " ", "\033[0m", __VA_ARGS__); \
  exit(1)

#define ensure(condition)               \
  do {                                  \
    if (!(condition)) {                 \
      debug_error("Assertion failed!"); \
    }                                   \
  } while (false);

struct ScopeTimer {
    std::string message;
    std::chrono::time_point<std::chrono::system_clock> start_time;

    ScopeTimer(std::string _message = "") : message(_message) {
      start_time = std::chrono::high_resolution_clock::now();
    }

    ~ScopeTimer() {
      auto end_time = std::chrono::high_resolution_clock::now();
      auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

      if (message.empty()) {
        debug_log_no_filename("took ", milliseconds);
      } else {
        debug_log_no_filename(message, " took ", milliseconds);
      }
    }
};

class Benchmark {
    static constexpr size_t max_measure_count = 1000;
    friend class measure;

  private:
    static inline std::unordered_map<std::string, std::vector<long>> times;
    static inline std::mutex times_mutex;

  public:
    static void print_all() {
      std::scoped_lock lock{times_mutex};
      for (auto& [tag, vec] : times) {
        debug_log_no_filename(tag);

        long sum = 0;

        for (auto ms : vec) {
          sum += ms;
        }
        if (vec.size() == max_measure_count) {
          debug_log_no_filename("\t count:   ", max_measure_count, "+");
        } else {
          debug_log_no_filename("\t count:   ", vec.size());
        }
        debug_log_no_filename("\t average: ", sum / (double)(vec.size()), " ms");
      }
    }

    class measure {
      public:
        [[nodiscard]] measure(std::string tag_) : tag{tag_} {
          start = std::chrono::high_resolution_clock::now();
        }

        ~measure() {
          auto end = std::chrono::high_resolution_clock::now();
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

          std::scoped_lock lock{Benchmark::times_mutex};

          auto it = Benchmark::times.find(tag);
          if (it != Benchmark::times.end()) {
            auto& my_times = it->second;

            if (my_times.size() == Benchmark::max_measure_count) {
              my_times[i] = ms;
              i = (i + 1) % Benchmark::max_measure_count;
            } else {
              my_times.emplace_back(ms);
            }

          } else {
            Benchmark::times[tag] = {ms};
          }
        }

      private:
        size_t i = 0;
        std::string tag;
        using chrono_time_point = decltype(std::chrono::high_resolution_clock::now());
        chrono_time_point start;
    };
};
#define CONCAT2__(a, b) a##b
#define CONCAT1__(a, b) CONCAT2__(a, b)
#define BENCHMARK(tag) auto CONCAT1__(benchmark_measure, __LINE__) = Benchmark::measure(tag);
