#include <csignal>
#include "core/settings.hpp"
#define STBI_ASSERT(x) ensure(x);
#define STBIW_ASSERT(x) ensure(x);
#define STBIR_ASSERT(x) ensure(x);

#include "opengl_includes.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <stacktrace>
#include <thread>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <nfd.hpp>
#include <unistd.h>
#include "common/config.hpp"
#include "common/debug.hpp"
#include "common/input.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/mpris.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/player.hpp"
#include "lib/json.cpp/json.h"
#include "ui/interface.hpp"

const char* get_opengl_error_string(GLenum err) {
  switch (err) {
  case GL_NO_ERROR: return "No error";
  case GL_INVALID_ENUM: return "Invalid enum";
  case GL_INVALID_VALUE: return "Invalid value";
  case GL_INVALID_OPERATION: return "Invalid operation";
  case GL_STACK_OVERFLOW: return "Stack overflow";
  case GL_STACK_UNDERFLOW: return "Stack underflow";
  case GL_OUT_OF_MEMORY: return "Out of memory";
  case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
  default: return "Unknown error";
  }
}

void check_opengl_errors() {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    std::stringstream error_hex;
    error_hex << std::hex << error << ": " << get_opengl_error_string(error);
    out::debug_error("GL error 0x{}", error_hex.str());
  }
}

void set_window_title(GLFWwindow* window) {
  static std::optional<player::playing_t> prev_playing;
  auto playing = player::get_playing();
  if (playing == prev_playing) { return; }
  prev_playing = playing;

  std::string window_title;
  if (playing.has_value()) {
    auto& track = db::track_by_id(playing->track_id)->get();
    window_title = "zincbox (" + utf32_to_utf8(track.pretty_name()) + ")";
  } else {
    window_title = "zincbox";
  }

  glfwSetWindowTitle(window, window_title.c_str());
}

std::atomic<bool> stop_flag = false;

extern "C" void handle_sigint(int signal) {
  if (signal == SIGINT) { stop_flag.store(true); }
}

extern "C" void handle_segfault(int) {
  out::log_critical("=== SEGMENTATION FAULT START ===");
  out::log_critical("stack trace (most recent call first):");

  std::string formatted_trace;
  int frame_num = 0;

  for (const auto& entry : std::stacktrace::current(1)) {
    std::string desc = entry.description();
    std::string file = entry.source_file();

    if (desc.empty()) { desc = "<unresolved symbol>"; }
    if (file.empty()) { file = "<unknown file>"; }

    formatted_trace += std::format("  #{:<2} {} \n      at {}:{}\n", frame_num++, desc, file, entry.source_line());
  }

  out::log_critical("\n{}", formatted_trace);
  out::log_critical("===  SEGMENTATION FAULT END  ===");
  std::exit(1);
}

int main() {
  if (std::signal(SIGINT, handle_sigint) == SIG_ERR) {
    out::log_critical("failed to set up signal handler for SIGINT");
    std::exit(1);
  }

  if (std::signal(SIGSEGV, handle_segfault) == SIG_ERR) {
    out::log_critical("failed to set up signal handler for SIGSEGV");
    std::exit(1);
  }

  config::load_from_file();
  if (config::json().contains("settings") && config::json()["settings"].isObject()) {
    settings::get().from_json(config::json()["settings"]);
  }

  NFD::Init();
  mpris::init();
  player::init();

  out::debug_info("deserialize start");
  if (std::filesystem::exists(io::get_db_path())) {
    auto s = std::ifstream{io::get_db_path(), std::ifstream::binary};
    db::deserialize(s);
  } else {
    db::create_empty_db();
  }
  out::debug_info("deserialize end");

  if (config::json().contains("player")) { player::from_json(config::json()["player"]); }

  // libdecor causes lag when resizing the window on Wayland
  // but it's needed for GNOME - so disable only if not needed
  const char* xdg_current_desktop = std::getenv("XDG_CURRENT_DESKTOP");
  if (std::strstr(xdg_current_desktop, "KDE") != nullptr || std::strstr(xdg_current_desktop, "GNOME") == nullptr) {
    glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);
  }
  if (!glfwInit()) {
    out::log_critical("failed to initialize GLFW");
    exit(1);
  }
  vec2i window_size = {
    std::clamp(config::get_i32("window_width").value_or(800), 480, 1920 * 4),
    std::clamp(config::get_i32("window_height").value_or(600), 320, 1080 * 4),
  };
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "zincbox", NULL, NULL);
  if (!window) {
    out::log_critical("failed to create window");
    exit(1);
  }
  glfwMakeContextCurrent(window);
  glfwSetWindowUserPointer(window, &window_size);
  glfwSetWindowSizeCallback(window, [](GLFWwindow* window_, int width, int height) -> void {
    glViewport(0, 0, width, height);
    auto* window_size_ = reinterpret_cast<glm::vec<2, int>*>(glfwGetWindowUserPointer(window_));
    window_size_->x = width;
    window_size_->y = height;
  });
  glfwSetWindowSizeLimits(window, 480, 320, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwGetWindowSize(window, &window_size.x, &window_size.y);
  int o = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if (o == 0) {
    out::log_critical("failed to load glad");
    exit(1);
  }
  // FIXME: glfwSwapBuffers hangs with  glfwSwapInterval(1), resulting in app not working
  // in the background possibly fixed by:
  // https://github.com/glfw/glfw/commit/413ba1dceb77f0d4552d565e7acc69a4379c6df8
  bool vsync = false;
  glfwSwapInterval(vsync ? 1 : 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Input::init(window);
  interface::init();
  if (config::json().contains("ui")) { interface::from_json(config::json()["ui"]); }

  while (!stop_flag) {
    using namespace std::chrono;
    auto t1 = high_resolution_clock::now();
    glfwPollEvents();
    Input::update();
    player::update();
    interface::input();
    interface::update(window_size);
    interface::draw();
    Input::clear();
    check_opengl_errors();
    set_window_title(window);
    glfwSwapBuffers(window);

    auto t2 = high_resolution_clock::now();
    long delta_us = duration_cast<microseconds>(t2 - t1).count();
    long sleep_us = std::max(1000.0, 16666.0 - delta_us);
    if (!vsync) { std::this_thread::sleep_for(microseconds(sleep_us)); }
    if (glfwWindowShouldClose(window)) { stop_flag = true; }
  }

  i32 maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
  config::set_i32("window_maximized", maximized);
  if (!maximized) {
    config::set_i32("window_width", window_size.x);
    config::set_i32("window_height", window_size.y);
  }

  {
    ScopeTimer x{"db::serialize"};
    auto s = std::ofstream{io::get_db_path(), std::ifstream::binary};
    db::serialize(s);
  }

  config::json()["player"] = player::to_json();
  config::json()["ui"] = interface::to_json();
  config::json()["settings"] = settings::get().to_json();
  config::save_to_file();
  interface::deinit();
  player::deinit();
  mpris::deinit();
}
