#include "common/logger.hpp"
#include <stacktrace>
#include <unordered_set>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include "common/types.hpp"

using enum out::LogLevel;

static std::unordered_set<std::string> filtered_files = {};

void out::add_file_to_blacklist(std::string_view filename) { filtered_files.emplace(filename); }

void out::remove_file_from_blacklist(const std::string& filename) { filtered_files.erase(filename); }

void out::detail::print_log(LogLevel level, std::string_view tag, std::string_view message) {
  auto tag_color = fmt::terminal_color::white;
  switch (level) {
  case INFO: tag_color = fmt::terminal_color::green; break;
  case WARNING: tag_color = fmt::terminal_color::yellow; break;
  case ERROR:
  case CRITICAL: tag_color = fmt::terminal_color::red; break;
  }

  fmt::print(fmt::emphasis::bold | fg(tag_color), "[{}] ", tag);
  if (level == CRITICAL) {
    fmt::print(fg(fmt::terminal_color::bright_red), "{}\n", message);
  } else {
    fmt::print("{}\n", message);
  }
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

  auto tag_color = fmt::terminal_color::white;
  switch (level) {
  case INFO: tag_color = fmt::terminal_color::green; break;
  case WARNING: tag_color = fmt::terminal_color::yellow; break;
  case ERROR:
  case CRITICAL: tag_color = fmt::terminal_color::red; break;
  }

  fmt::print(fmt::emphasis::bold | fg(tag_color), "[{}:{}] ", filename, line_number);
  if (level == CRITICAL) {
    fmt::print(fg(fmt::terminal_color::bright_red), "{}\n", message);
  } else {
    fmt::print("{}\n", message);
  }
}
