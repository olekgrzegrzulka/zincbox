#include "common/logger.hpp"
#include <stacktrace>
#include <unordered_set>
#include <fmt/core.h>
#include <fmt/format.h>
#include "common/types.hpp"

namespace style {
  static constexpr std::string normal = "\033[0m";
  static constexpr std::string bold = "\033[1m";
} // namespace style

namespace color {
  static constexpr std::string red = "\033[31m";
  static constexpr std::string green = "\033[32m";
  static constexpr std::string yellow = "\033[33m";
  static constexpr std::string blue = "\033[34m";
  static constexpr std::string magenta = "\033[35m";
  static constexpr std::string normal = "\033[39m";
} // namespace color

using enum out::LogLevel;

static std::unordered_set<std::string> filtered_files = {};

void out::add_file_to_blacklist(std::string_view filename) { filtered_files.emplace(filename); }

void out::remove_file_from_blacklist(const std::string& filename) { filtered_files.erase(filename); }

void out::detail::print_log(LogLevel level, std::string_view tag, std::string_view message) {
  auto color = color::blue;
  switch (level) {
  case INFO: color = color::green; break;
  case WARNING: color = color::yellow; break;
  case ERROR: color = color::red; break;
  case CRITICAL: color = color::magenta; break;
  }

  fmt::print("{}{}[{}] {}{}{}\n", style::bold, color, tag, color::normal, style::normal, message);
}

void out::detail::debug_log(LogLevel level, std::string_view message, size_t skip) {
  auto stack_current = std::stacktrace::current();
  std::string filename = "?";
  u32 line_number = 0;

  skip += 1;

  if (skip < stack_current.size()) {
    auto source_file = stack_current[skip].source_file();
    auto last_slash = source_file.find_last_of('/');
    filename = source_file.substr(last_slash == std::string::npos ? 0 : last_slash + 1);
    line_number = stack_current[skip].source_line();
  }

  if (filtered_files.contains(filename)) { return; }

  auto color = color::blue;
  switch (level) {
  case INFO: color = color::green; break;
  case WARNING: color = color::yellow; break;
  case ERROR: color = color::red; break;
  case CRITICAL: color = color::magenta; break;
  }

  if (level == CRITICAL) {
    // red text
    fmt::print("{}{}[{}:{}] {}{}{}{}\n", style::bold, color, filename, line_number, style::normal, color::red, message,
               color::normal);
  } else {
    // normal text
    fmt::print("{}{}[{}:{}] {}{}{}\n", style::bold, color, filename, line_number, color::normal, style::normal,
               message);
  }
}
