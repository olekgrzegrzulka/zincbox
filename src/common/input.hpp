#pragma once
#include <string>
#include <variant>
#include <vector>
#include <glm/vec2.hpp>
#include "common/types.hpp"

struct GLFWwindow;

namespace Input {

  enum class Key : u16 {
    /* Printable keys */
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39 /* ' */,
    KEY_COMMA = 44 /* , */,
    KEY_MINUS = 45 /* - */,
    KEY_PERIOD = 46 /* . */,
    KEY_SLASH = 47 /* / */,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_SEMICOLON = 59 /* ; */,
    KEY_EQUAL = 61 /* = */,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91 /* [ */,
    KEY_BACKSLASH = 92 /* \ */,
    KEY_RIGHT_BRACKET = 93 /* ] */,
    KEY_GRAVE_ACCENT = 96 /* ` */,
    KEY_WORLD_1 = 161 /* non-US #1 */,
    KEY_WORLD_2 = 162 /* non-US #2 */,

    /* Function keys */
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,
    KEY_F13 = 302,
    KEY_F14 = 303,
    KEY_F15 = 304,
    KEY_F16 = 305,
    KEY_F17 = 306,
    KEY_F18 = 307,
    KEY_F19 = 308,
    KEY_F20 = 309,
    KEY_F21 = 310,
    KEY_F22 = 311,
    KEY_F23 = 312,
    KEY_F24 = 313,
    KEY_F25 = 314,
    KEY_KP_0 = 320,
    KEY_KP_1 = 321,
    KEY_KP_2 = 322,
    KEY_KP_3 = 323,
    KEY_KP_4 = 324,
    KEY_KP_5 = 325,
    KEY_KP_6 = 326,
    KEY_KP_7 = 327,
    KEY_KP_8 = 328,
    KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330,
    KEY_KP_DIVIDE = 331,
    KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333,
    KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335,
    KEY_KP_EQUAL = 336,
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_MENU = 348,
    KEY_LAST = KEY_MENU,
    KEY_SIZE = KEY_LAST + 1
  };

  enum class KeyAction : u8 { PRESS, RELEASE, REPEAT };

  enum class MouseButton : u8 {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_4,
    MOUSE_BUTTON_5,
    MOUSE_BUTTON_6,
    MOUSE_BUTTON_7,
    MOUSE_BUTTON_8,
    MOUSE_BUTTON_SIZE,
  };

  enum class MouseAction : u8 { PRESS, RELEASE };

  enum class Cursor : u8 {
    ARROW,
    IBEAM,
    CROSSHAIR,
    POINTING_HAND,
    RESIZE_HORIZONTAL,
    RESIZE_VERTICAL,
    RESIZE,
    NOT_ALLOWED,
    HAND,
    CURSOR_SIZE,
  };

  struct InputEventMouseButton {
      MouseButton button{};
      MouseAction action{};
      bool handled = false;
  };

  struct InputEventMouseMove {
      vec2f to{};
      bool handled = false;
  };

  struct InputEventMouseScroll {
      vec2f offset{};
      bool handled = false;
  };

  struct InputEventKey {
      Key key;
      KeyAction action;
      i32 scancode;
      bool handled = false;
  };

  struct InputEventMouseEntered {
      bool handled = false;
  };

  struct InputEventMouseLeft {
      bool handled = false;
  };

  struct InputEventCloseWindow {
      bool handled = false;
  };

  using InputEvent = std::variant<InputEventMouseButton, InputEventMouseMove, InputEventMouseScroll, InputEventKey,
                                  InputEventMouseEntered, InputEventMouseLeft
                                  // InputEventCloseWindow
                                  >;

  void glfw_cursor_position_callback(GLFWwindow*, double x, double y);
  void glfw_mouse_button_callback(GLFWwindow*, i32 button, i32 action, i32 mods);
  void glfw_scroll_button_callback(GLFWwindow*, double xoffset, double yoffset);
  void glfw_char_callback(GLFWwindow*, u32 c);
  void glfw_key_callback(GLFWwindow*, i32 key, i32 scancode, i32 action, i32 mods);
  void glfw_cursor_enter_callback(GLFWwindow*, i32 entered);
  void glfw_close_window_callback(GLFWwindow*);
  void glfw_drop_callback(GLFWwindow* window, int count, const char** paths);

  void init(GLFWwindow*);

  void update();

  void clear();

  vec2i get_mouse_pos();
  i32 get_mouse_x();
  i32 get_mouse_y();
  vec2f get_mouse_scroll();
  vec2i get_mouse_delta();

  i32 get_window_x();
  i32 get_window_y();

  std::vector<InputEvent>& get_event_queue();

  bool mouse_pressed(Input::MouseButton);
  bool mouse_just_pressed(Input::MouseButton);
  bool mouse_just_released(Input::MouseButton);

  bool key_pressed(Input::Key);
  bool key_just_pressed(Input::Key);
  bool key_just_released(Input::Key);

  vec2i get_window_size();

  std::u32string get_typed_characters();

  std::string key_to_string(Input::Key key);

  const std::vector<std::string>& get_dropped_paths();

  void reset_cursor();
  void set_cursor(Input::Cursor cursor);
}; // namespace Input
