#pragma once
#include <string>
#include <string_view>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include "common/types.hpp"

#define DEFINE_LOG_FUNC(FUNC_NAME, LEVEL, TAG)                                                                         \
  template <typename... Args> void FUNC_NAME(fmt::format_string<Args...> fmt, Args&&... args) {                        \
    detail::print_log(LogLevel::LEVEL, TAG, fmt::format(fmt, std::forward<Args>(args)...));                            \
  }                                                                                                                    \
  template <typename T> void FUNC_NAME(const T& value) { FUNC_NAME("{}", value); }

#ifndef NDEBUG
#define DEFINE_DEBUG_FUNC(FUNC_NAME, LEVEL)                                                                            \
  template <typename... Args> void FUNC_NAME(fmt::format_string<Args...> fmt, Args&&... args) {                        \
    detail::debug_log(LogLevel::LEVEL, fmt::format(fmt, std::forward<Args>(args)...), 1);                              \
  }                                                                                                                    \
  template <typename T> void FUNC_NAME(const T& value) { FUNC_NAME("{}", value); }
#else
#define DEFINE_DEBUG_FUNC(FUNC_NAME, LEVEL)                                                                            \
  template <typename... Args>                                                                                          \
  void FUNC_NAME([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) {}                 \
  template <typename T> void FUNC_NAME([[maybe_unused]] const T& value) {}
#endif

namespace out {
  void add_file_to_blacklist(std::string_view filename);
  void remove_file_from_blacklist(const std::string& filename);

  enum class LogLevel : u8 { INFO, WARNING, ERROR, CRITICAL };

  namespace detail {
    void print_log(LogLevel level, std::string_view tag, std::string_view message);
    void debug_log(LogLevel level, std::string_view message, size_t skip);
  } // namespace detail

  template <typename... Args> void println(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::println("{}", fmt::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args> void print(fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::print("{}", fmt::format(fmt, std::forward<Args>(args)...));
  }

  DEFINE_LOG_FUNC(info, INFO, "info")
  DEFINE_LOG_FUNC(warn, WARNING, "warning")
  DEFINE_LOG_FUNC(error, ERROR, "error")
  DEFINE_LOG_FUNC(critical, CRITICAL, "critical")

  DEFINE_DEBUG_FUNC(debug_info, INFO)
  DEFINE_DEBUG_FUNC(debug_warn, WARNING)
  DEFINE_DEBUG_FUNC(debug_error, ERROR)
  DEFINE_DEBUG_FUNC(debug_critical, CRITICAL)

} // namespace out
