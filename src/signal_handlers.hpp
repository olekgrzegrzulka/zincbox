#include <cstdlib>
#include <stacktrace>
#include <string>
#include <signal.h>
#include "common/logger.hpp"
#include "core/zincbox.hpp"

extern "C" void handle_sigterm(int signal) {
  if (signal == SIGTERM) {
    out::warn("received SIGTERM, shutting down");
    zincbox::stop();
  }
}

extern "C" void handle_sighup(int signal) {
  if (signal == SIGHUP) {
    out::warn("received SIGHUP, shutting down");
    zincbox::stop();
  }
}

extern "C" void handle_sigint(int signal) {
  if (signal == SIGINT) {
    out::warn("received SIGINT, shutting down");
    zincbox::stop();
  }
}

extern "C" void handle_sigsegv(int) {
  out::critical("=== SEGMENTATION FAULT START ===");
  out::critical("stack trace (most recent call first):");

  std::string formatted_trace;
  int frame_num = 0;

  for (const auto& entry : std::stacktrace::current(1)) {
    std::string desc = entry.description();
    std::string file = entry.source_file();

    if (desc.empty()) { desc = "<unresolved symbol>"; }
    if (file.empty()) { file = "<unknown file>"; }

    formatted_trace += std::format("  #{:<2} {} \n      at {}:{}\n", frame_num++, desc, file, entry.source_line());
  }

  out::critical("\n{}", formatted_trace);
  out::critical("===  SEGMENTATION FAULT END  ===");
  std::exit(1);
}
