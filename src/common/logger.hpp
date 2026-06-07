#pragma once
#include <string>
#include <string_view>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include "common/types.hpp"

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

  template <typename... Args> void log_info(fmt::format_string<Args...> fmt, Args&&... args) {
    detail::print_log(LogLevel::INFO, "info", fmt::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args> void log_warning(fmt::format_string<Args...> fmt, Args&&... args) {
    detail::print_log(LogLevel::WARNING, "warning", fmt::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args> void log_error(fmt::format_string<Args...> fmt, Args&&... args) {
    detail::print_log(LogLevel::ERROR, "error", fmt::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args> void log_critical(fmt::format_string<Args...> fmt, Args&&... args) {
    detail::print_log(LogLevel::CRITICAL, "critical", fmt::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args> void debug_info(fmt::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
    detail::debug_log(LogLevel::INFO, fmt::format(fmt, std::forward<Args>(args)...), 1);
#endif
  }

  template <typename... Args> void debug_warning(fmt::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
    detail::debug_log(LogLevel::WARNING, fmt::format(fmt, std::forward<Args>(args)...), 1);
#endif
  }

  template <typename... Args> void debug_error(fmt::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
    detail::debug_log(LogLevel::ERROR, fmt::format(fmt, std::forward<Args>(args)...), 1);
#endif
  }

  template <typename... Args> void debug_critical(fmt::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
    detail::debug_log(LogLevel::CRITICAL, fmt::format(fmt, std::forward<Args>(args)...), 1);
#endif
  }

} // namespace out
