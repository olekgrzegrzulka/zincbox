#include "common/logger.hpp"
#include <cstdio>
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
  FILE* stream = stdout;
  switch (level) {
  case INFO: tag_color = fmt::terminal_color::green; break;
  case WARNING: tag_color = fmt::terminal_color::yellow; break;
  case ERROR: [[fallthrough]];
  case CRITICAL:
    stream = stderr;
    tag_color = fmt::terminal_color::red;
    break;
  }

  fmt::print(stream, fmt::emphasis::bold | fg(tag_color), "[{}] ", tag);
  if (level == CRITICAL) {
    fmt::print(stream, fg(fmt::terminal_color::bright_red), "{}\n", message);
  } else {
    fmt::print(stream, "{}\n", message);
  }
}

void out::detail::debug_log(LogLevel level, std::string_view message, size_t /* skip */) {
  auto stack_current = std::stacktrace::current();
  std::string filename = "?";
  u32 line_number = 0;

  for (const auto& frame : stack_current) {
    auto source_file = frame.source_file();
    if (source_file.empty() || source_file.ends_with("logger.cpp") || source_file.ends_with("logger.hpp")) { continue; }
    auto last_slash = source_file.find_last_of("/\\");
    filename = source_file.substr(last_slash == std::string::npos ? 0 : last_slash + 1);
    line_number = frame.source_line();
    break;
  }

  if (filtered_files.contains(filename)) { return; }

  auto tag_color = fmt::terminal_color::white;
  FILE* stream = stdout;
  switch (level) {
  case INFO: tag_color = fmt::terminal_color::green; break;
  case WARNING: tag_color = fmt::terminal_color::yellow; break;
  case ERROR:
  case CRITICAL:
    stream = stderr;
    tag_color = fmt::terminal_color::red;
    break;
  }

  fmt::print(stream, fmt::emphasis::bold | fg(tag_color), "[{}:{}] ", filename, line_number);
  if (level == CRITICAL) {
    fmt::print(stream, fg(fmt::terminal_color::bright_red), "{}\n", message);
  } else {
    fmt::print(stream, "{}\n", message);
  }
}
