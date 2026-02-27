#include "interface.hpp"
#include "opengl_includes.hpp"

#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <nfd.hpp>
#include <unistd.h>
#include "config.hpp"
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "player.hpp"

#define STBI_ASSERT(x) ensure(x);
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define STBIW_ASSERT(x) ensure(x);
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#define STBIR_ASSERT(x) ensure(x);
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../lib/stb_image_resize2.h"
#undef STB_IMAGE_RESIZE_IMPLEMENTATION

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
  player::init();
  config_load_from_file("music.cfg");

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
  glfwSwapInterval(1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Input::init(window);
  interface::init();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    Input::update();
    player::update();
    interface::process_input();
    interface::update(window_size);
    interface::draw();
    Input::clear();
    check_opengl_errors();
    glfwSwapBuffers(window);
  }

  i32 maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
  config_set_i32("window_maximized", maximized);
  if (!maximized) {
    config_set_i32("window_width", window_size.x);
    config_set_i32("window_height", window_size.y);
  }
  std::ofstream os("musicdb", std::ios::binary);
  musicdb::save_collections_to_file(os);
  config_save_to_file("music.cfg");
  interface::deinit();
  player::deinit();
}
