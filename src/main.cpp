#define STBI_ASSERT(x) ensure(x);
#define STBIW_ASSERT(x) ensure(x);
#define STBIR_ASSERT(x) ensure(x);

#include "opengl_includes.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <thread>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <nfd.hpp>
#include <unistd.h>
#include "common/config.hpp"
#include "common/debug.hpp"
#include "common/input.hpp"
#include "core/mpris.hpp"
#include "core/musicdb.hpp"
#include "core/player.hpp"
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
    debug_warn("GL error 0x", error_hex.str());
  }
}

int main() {
  std::cout << std::setprecision(2) << std::fixed << std::showpoint << std::boolalpha;
  NFD::Init();
  mpris::init();
  player::init();
  mpris::init();
  config_load_from_file("music.cfg");
  i32 repeat_mode = config_get_i32("repeat_mode").value_or(0);
  repeat_mode = std::clamp(repeat_mode, 0, (i32)player::RepeatMode::REPEAT_MODE_SIZE - 1);
  player::set_repeat_mode((player::RepeatMode)repeat_mode);
  i32 shuffle_mode = config_get_i32("shuffle_mode").value_or(0);
  shuffle_mode = std::clamp(shuffle_mode, 0, (i32)player::ShuffleMode::SHUFFLE_MODE_SIZE - 1);
  player::set_shuffle_mode((player::ShuffleMode)shuffle_mode);
  float volume = std::clamp(config_get_float("volume").value_or(0.5f), 0.0f, 1.0f);
  player::set_volume(volume);

  if (std::filesystem::exists("musicdb")) {
    auto s = std::ifstream{"musicdb", std::ifstream::binary};
    db::deserialize(s);
  } else {
    db::add_collection(U"Playlists");
    auto& collection = db::collection_by_id(0)->get();
    collection.add_playlist(U"Loved tracks", U"");
  }

  glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR); // libdecor causes lag when resizing the window on Wayland
  if (!glfwInit()) { debug_error("Failed to initialzie GLFW"); }
  vec2i window_size = {
    std::clamp(config_get_i32("window_width").value_or(800), 480, 1920 * 4),
    std::clamp(config_get_i32("window_height").value_or(600), 320, 1080 * 4),
  };
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "music", NULL, NULL);
  if (!window) { debug_error("Failed to create window"); }
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
  if (o == 0) { debug_error("failed to load GLAD loader"); }
  // FIXME: glfwSwapBuffers hangs with  glfwSwapInterval(1), resulting in app not working in the background
  // possibly fixed by: https://github.com/glfw/glfw/commit/413ba1dceb77f0d4552d565e7acc69a4379c6df8
  bool vsync = false;
  glfwSwapInterval(vsync ? 1 : 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Input::init(window);
  interface::init();

  while (!glfwWindowShouldClose(window)) {
    using namespace std::chrono;
    auto t1 = high_resolution_clock::now();
    glfwPollEvents();
    Input::update();
    player::update();
    interface::process_input();
    interface::update(window_size);
    interface::draw();
    Input::clear();
    check_opengl_errors();
    glfwSwapBuffers(window);

    auto t2 = high_resolution_clock::now();
    long delta_us = duration_cast<microseconds>(t2 - t1).count();
    // std::cout << "delta = " << duration_cast<milliseconds>(t2 - t1).count() << " ms\n";
    long sleep_us = std::max(1000.0, 16666.0 - delta_us);
    if (!vsync) { std::this_thread::sleep_for(microseconds(sleep_us)); }
  }

  i32 maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
  config_set_i32("window_maximized", maximized);
  if (!maximized) {
    config_set_i32("window_width", window_size.x);
    config_set_i32("window_height", window_size.y);
  }

  {
    ScopeTimer x{"db::serialize"};
    auto s = std::ofstream{"musicdb", std::ifstream::binary};
    db::serialize(s);
  }

  config_set_i32("repeat_mode", (i32)player::get_repeat_mode());
  config_set_i32("shuffle_mode", (i32)player::get_shuffle_mode());
  config_set_float("volume", player::get_volume());
  config_save_to_file("music.cfg");
  interface::deinit();
  player::deinit();
  mpris::deinit();
}
