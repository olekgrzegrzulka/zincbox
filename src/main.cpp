#include "musicdb_serialize.hpp"
#include "opengl_includes.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <nfd.hpp>
#include <unistd.h>
#include "bridge.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "panel_albums.hpp"
#include "panel_controls.hpp"
#include "panel_tracks.hpp"
#include "player.hpp"
#include "seekbar.hpp"
#include "texture_atlas.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/sprite.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

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
  NFD::Init();

  player::init();

  std::cout << std::setprecision(2) << std::fixed << std::showpoint << std::boolalpha;

  config_load_from_file("music.cfg");

  glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR); // libdecor causes lag when resizing the window on Wayland
  if (!glfwInit()) {
    debug_error("Failed to initialzie GLFW");
  }

  vec2i window_size = {
    std::clamp(config_get_i32("window_width").value_or(800), 480, 1920 * 4),
    std::clamp(config_get_i32("window_height").value_or(600), 320, 1080 * 4),
  };

  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "music", NULL, NULL);
  if (!window) {
    debug_error("Failed to create window");
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
    debug_error("failed to load GLAD loader");
  }

  Input::init(window);

  UI ui = UI(1, 1);

  auto& atlas = ui.get_texture_atlas();
  atlas.add_texture("button_disabled", "./assets/button_disabled.png");
  atlas.add_texture("button_hovered", "./assets/button_hovered.png");
  atlas.add_texture("button_idle", "./assets/button_idle.png");
  atlas.add_texture("button_pressed", "./assets/button_pressed.png");
  atlas.add_texture("combo_box_button_contract", "./assets/combo_box_button_contract.png");
  atlas.add_texture("combo_box_button_expand", "./assets/combo_box_button_expand.png");
  atlas.add_texture("combo_box", "./assets/combo_box.png");
  atlas.add_texture("dim", "./assets/dim.png");
  atlas.add_texture("panel_rectangular_highlighted", "./assets/panel_rectangular_highlighted.png");
  atlas.add_texture("panel_rectangular", "./assets/panel_rectangular.png");
  atlas.add_texture("panel_rectangular_dark", "./assets/panel_rectangular_dark.png");
  atlas.add_texture("panel_rounded_dark", "./assets/panel_rounded_dark.png");
  atlas.add_texture("panel_rounded_light", "./assets/panel_rounded_light.png");
  atlas.add_texture("panel_rectangular_light", "./assets/panel_rectangular_light.png");
  atlas.add_texture("panel_rounded", "./assets/panel_rounded.png");
  atlas.add_texture("panel_shadow", "./assets/panel_shadow.png");
  atlas.add_texture("red", "./assets/red.png");
  atlas.add_texture("selectbar_bg", "./assets/selectbar_bg.png");
  atlas.add_texture("selectbar_selected", "./assets/selectbar_selected.png");
  atlas.add_texture("slider_thumb_hovered", "./assets/slider_thumb_hovered.png");
  atlas.add_texture("slider_thumb_idle", "./assets/slider_thumb_idle.png");
  atlas.add_texture("slider_thumb_pressed", "./assets/slider_thumb_pressed.png");
  atlas.add_texture("slider_track", "./assets/slider_track.png");
  atlas.add_texture("scrollbar_thumb_hovered", "./assets/scrollbar_thumb_hovered.png");
  atlas.add_texture("scrollbar_thumb_idle", "./assets/scrollbar_thumb_idle.png");
  atlas.add_texture("scrollbar_thumb_pressed", "./assets/scrollbar_thumb_pressed.png");
  atlas.add_texture("scrollbar_track", "./assets/scrollbar_track.png");
  atlas.add_texture("spinner_buttons", "./assets/spinner_buttons.png");
  atlas.add_texture("text_input_caret", "./assets/text_input_caret.png");
  atlas.add_texture("text_input_focused", "./assets/text_input_focused.png");
  atlas.add_texture("text_input_idle", "./assets/text_input_idle.png");
  atlas.add_texture("seekbar_bg", "./assets/seekbar_bg.png");
  atlas.add_texture("seekbar_progress", "./assets/seekbar_progress.png");
  atlas.add_texture("seekbar_thumb", "./assets/seekbar_thumb.png");
  atlas.add_texture("track_bg1", "./assets/track_bg1.png");
  atlas.add_texture("track_bg2", "./assets/track_bg2.png");
  atlas.add_texture("track_bg_playing", "./assets/track_bg_playing.png");

  atlas.add_texture("play", "./assets/icons/play.png");
  atlas.add_texture("pause", "./assets/icons/pause.png");
  atlas.add_texture("stop", "./assets/icons/stop.png");
  atlas.add_texture("next", "./assets/icons/next.png");
  atlas.add_texture("prev", "./assets/icons/prev.png");
  atlas.add_texture("repeat", "./assets/icons/repeat.png");
  atlas.add_texture("repeat_off", "./assets/icons/repeat_off.png");
  atlas.add_texture("repeat_album", "./assets/icons/repeat_album.png");
  atlas.add_texture("repeat_track", "./assets/icons/repeat_track.png");
  atlas.add_texture("shuffle", "./assets/icons/shuffle.png");

  atlas.save_to_file("atlas.png");

  auto& panel_top = ui.add_widget<Panel>();
  panel_top.set_height(40);
  panel_top.set_layout("m:4 s:4 ltr expand");

  auto& panel_bottom = ui.add_widget<PanelControls>();

  auto& panel_main = ui.add_widget<Panel>();
  panel_main.set_layout("m:0 s:0 ltr expand fill");

  auto& panel_left = panel_main.add_child<PanelTracks>();
  auto& panel_right = panel_main.add_child<PanelAlbums>();

  auto& button_scan = panel_top.add_child<Button>("Scan for music");
  button_scan.set_width(120);
  auto& button_load = panel_top.add_child<Button>("Load from file");
  button_load.set_width(120);
  auto& button_save = panel_top.add_child<Button>("Save to file");
  button_save.set_width(120);

  bridge::init(&panel_left, &panel_right);

  button_scan.on_press([&]() {
    musicdb::load("/home/olek/Muzyka/");

    panel_left.recreate();
    panel_right.recreate();
  });

  button_save.on_press([&]() {
    musicdb::save_to_file("musicdb");
  });

  button_load.on_press([&]() {
    musicdb::load_from_file("musicdb");

    panel_left.recreate();
    panel_right.recreate();
  });

  glfwSwapInterval(1);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (!glfwWindowShouldClose(window)) {
    // auto start_time = std::chrono::high_resolution_clock::now();
    glfwPollEvents();
    Input::update();
    ui.process_input();
    ui.update(window_size.x, window_size.y);
    ui.draw();

    panel_top.set_width(window_size.x);
    panel_main.set_width(window_size.x);
    panel_bottom.set_width(window_size.x);

    panel_main.set_y(panel_top.get_height());
    panel_main.set_height(window_size.y - panel_top.get_height() - panel_bottom.get_height());

    // FIXME: run this on different thread as it doesn't run when window is minimized!
    player::update();

    // redrawn.input();
    Input::clear();

    check_opengl_errors();
    glfwSwapBuffers(window);

    // auto end_time = std::chrono::high_resolution_clock::now();
    // uint16_t delta = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    // usleep(std::max(delta - 16666, 0));
  }

  i32 maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
  config_set_i32("window_maximized", maximized);
  if (!maximized) {
    config_set_i32("window_width", window_size.x);
    config_set_i32("window_height", window_size.y);
  }

  config_save_to_file("music.cfg");
  player::deinit();
}
