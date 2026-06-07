#include "input.hpp"
#include <algorithm>
#include <array>
#include <variant>
#include <vector>
#include "common/debug.hpp"
#include "common/types.hpp"
#include "opengl_includes.hpp"

namespace Input {

  namespace detail {
    static GLFWwindow* glfw_window{};
    static i32 mouse_x{};
    static i32 mouse_y{};
    static i32 last_mouse_x{};
    static i32 last_mouse_y{};
    static i32 window_x{};
    static i32 window_y{};
    static std::vector<InputEvent> event_queue{};
    static std::vector<std::string> dropped_paths{};
    static std::array<GLFWcursor*, (size_t)Cursor::CURSOR_SIZE> cursors{};

    enum class ButtonState {
      RELEASED,
      PRESSED,
      JUST_RELEASED,
      JUST_PRESSED,
    };

    std::array<ButtonState, (size_t)MouseButton::MOUSE_BUTTON_SIZE> mouse_states;
    std::array<ButtonState, (size_t)Key::KEY_SIZE> key_states;
    std::u32string keyboard_characters_prev;
    std::u32string keyboard_characters_curr;
    vec2f accumulated_scroll_next{};
    vec2f accumulated_scroll{};
  } // namespace detail

  void glfw_cursor_position_callback(GLFWwindow*, double x, double y) {
    InputEventMouseMove ev{.to = {x, y}};
    detail::event_queue.emplace_back(ev);
  }

  void glfw_mouse_button_callback(GLFWwindow*, i32 button, i32 action, i32 /* mods */) {
    InputEventMouseButton ev = {
      .button = static_cast<MouseButton>(button),
      .action = (action == GLFW_PRESS) ? MouseAction::PRESS : MouseAction::RELEASE,
    };
    detail::event_queue.emplace_back(ev);
  }

  void glfw_drop_callback(GLFWwindow*, i32 count, const char** paths) {
    detail::dropped_paths.clear();
    detail::dropped_paths.resize(count);
    for (i32 i = 0; i < count; i += 1) {
      detail::dropped_paths[i] = std::string{paths[i]};
    }
  }

  void glfw_scroll_button_callback(GLFWwindow*, double x, double y) {
    detail::accumulated_scroll_next += vec2f{x, y};

    InputEventMouseScroll ev = {
      .offset = {x, y},
    };
    detail::event_queue.emplace_back(ev);
  }

  void glfw_char_callback(GLFWwindow*, u32 c) { detail::keyboard_characters_curr += c; }

  void glfw_key_callback(GLFWwindow*, i32 key, i32 scancode, i32 action, i32) {
    KeyAction key_action{};
    if (action == GLFW_PRESS) { key_action = KeyAction::PRESS; }
    if (action == GLFW_REPEAT) { key_action = KeyAction::REPEAT; }
    if (action == GLFW_RELEASE) { key_action = KeyAction::RELEASE; }
    InputEventKey ev = {
      .key = static_cast<Key>(key),
      .action = key_action,
      .scancode = scancode,
    };
    detail::event_queue.emplace_back(ev);
  }

  void glfw_cursor_enter_callback(GLFWwindow*, i32 entered) {
    if (entered == GLFW_TRUE) {
      detail::event_queue.emplace_back(InputEventMouseEntered{});
    } else {
      detail::event_queue.emplace_back(InputEventMouseLeft{});
    }
  }

  void glfw_close_window_callback(GLFWwindow*) {
    // detail::event_queue.emplace_back(InputEventCloseWindow{});
  }

  void init(GLFWwindow* window) {
    detail::glfw_window = window;
    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetScrollCallback(window, glfw_scroll_button_callback);
    glfwSetCharCallback(window, glfw_char_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetCursorEnterCallback(window, glfw_cursor_enter_callback);
    glfwSetWindowCloseCallback(window, glfw_close_window_callback);
    glfwSetDropCallback(window, glfw_drop_callback);

    detail::cursors[(size_t)Cursor::ARROW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    detail::cursors[(size_t)Cursor::IBEAM] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    detail::cursors[(size_t)Cursor::CROSSHAIR] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    detail::cursors[(size_t)Cursor::HAND] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    detail::cursors[(size_t)Cursor::RESIZE_HORIZONTAL] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    detail::cursors[(size_t)Cursor::RESIZE_VERTICAL] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    detail::cursors[(size_t)Cursor::RESIZE] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    detail::cursors[(size_t)Cursor::NOT_ALLOWED] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
  }

  void update() {
    double x, y;
    glfwGetWindowPos(detail::glfw_window, &detail::window_x, &detail::window_y);
    glfwGetCursorPos(detail::glfw_window, &x, &y);
    detail::last_mouse_x = detail::mouse_x;
    detail::last_mouse_y = detail::mouse_y;
    detail::mouse_x = (i32)x;
    detail::mouse_y = (i32)y;

    for (size_t i = 0; i < detail::mouse_states.size(); i += 1) {
      using enum detail::ButtonState;
      bool is_pressed = glfwGetMouseButton(detail::glfw_window, i) == GLFW_PRESS;

      if (detail::mouse_states[i] == RELEASED && is_pressed) {
        detail::mouse_states[i] = JUST_PRESSED;
      } else if (detail::mouse_states[i] == PRESSED && !is_pressed) {
        detail::mouse_states[i] = JUST_RELEASED;
      } else if (detail::mouse_states[i] == JUST_RELEASED) {
        if (!is_pressed) {
          detail::mouse_states[i] = RELEASED;
        } else {
          detail::mouse_states[i] = JUST_PRESSED;
        }
      } else if (detail::mouse_states[i] == JUST_PRESSED) {
        if (is_pressed) {
          detail::mouse_states[i] = PRESSED;
        } else {
          detail::mouse_states[i] = JUST_RELEASED;
        }
      }
    }

    for (size_t i = 0; i < detail::key_states.size(); i += 1) {
      using enum detail::ButtonState;
      bool is_pressed = glfwGetKey(detail::glfw_window, i) == GLFW_PRESS;

      if (detail::key_states[i] == RELEASED && is_pressed) {
        detail::key_states[i] = JUST_PRESSED;
      } else if (detail::key_states[i] == PRESSED && !is_pressed) {
        detail::key_states[i] = JUST_RELEASED;
      } else if (detail::key_states[i] == JUST_RELEASED) {
        if (!is_pressed) {
          detail::key_states[i] = RELEASED;
        } else {
          detail::key_states[i] = JUST_PRESSED;
        }
      } else if (detail::key_states[i] == JUST_PRESSED) {
        if (is_pressed) {
          detail::key_states[i] = PRESSED;
        } else {
          detail::key_states[i] = JUST_RELEASED;
        }
      }
    }
    // clang-format on
  }

  void clear() {
    detail::keyboard_characters_prev = detail::keyboard_characters_curr;
    detail::keyboard_characters_curr.clear();

    detail::accumulated_scroll = detail::accumulated_scroll_next;
    detail::accumulated_scroll_next = vec2f{};

    detail::event_queue.clear();

    detail::dropped_paths.clear();
  }

  vec2i get_mouse_pos() { return {detail::mouse_x, detail::mouse_y}; }

  i32 get_mouse_x() { return detail::mouse_x; }

  i32 get_mouse_y() { return detail::mouse_y; }

  i32 get_window_x() { return detail::window_x; }

  i32 get_window_y() { return detail::window_y; }

  std::vector<InputEvent>& get_event_queue() {
    auto new_end = std::remove_if(detail::event_queue.begin(), detail::event_queue.end(), [](const InputEvent& ev) {
      return std::visit([](auto&& e) { return e.handled; }, ev);
    });
    detail::event_queue.erase(new_end, detail::event_queue.end());
    return detail::event_queue;
  }

  bool mouse_pressed(MouseButton button) {
    using enum detail::ButtonState;
    auto state = detail::mouse_states[(size_t)button];
    return state == JUST_PRESSED || state == PRESSED;
  }

  bool mouse_just_pressed(MouseButton button) {
    return detail::mouse_states[(size_t)button] == detail::ButtonState::JUST_PRESSED;
  }

  bool mouse_just_released(MouseButton button) {
    return detail::mouse_states[(size_t)button] == detail::ButtonState::JUST_RELEASED;
  }

  bool key_pressed(Key button) {
    using enum detail::ButtonState;
    auto state = detail::key_states[(size_t)button];
    return state == JUST_PRESSED || state == PRESSED;
  }

  bool key_just_pressed(Key button) { return detail::key_states[(size_t)button] == detail::ButtonState::JUST_PRESSED; }

  bool key_just_released(Key button) {
    return detail::key_states[(size_t)button] == detail::ButtonState::JUST_RELEASED;
  }

  vec2f get_mouse_scroll() { return detail::accumulated_scroll; }

  vec2i get_mouse_delta() { return {detail::mouse_x - detail::last_mouse_x, detail::mouse_y - detail::last_mouse_y}; }

  vec2i get_window_size() {
    i32 width = 0;
    i32 height = 0;
    glfwGetWindowSize(detail::glfw_window, &width, &height);
    return {width, height};
  }

  std::u32string get_typed_characters() { return detail::keyboard_characters_prev; }

  std::string key_to_string(Input::Key key) {
    switch ((i32)key) {
    case 32: return "SPACE";
    case 39: return "APOSTROPHE";
    case 44: return "COMMA";
    case 45: return "MINUS";
    case 46: return "PERIOD";
    case 47: return "SLASH";
    case 48: return "0";
    case 49: return "1";
    case 50: return "2";
    case 51: return "3";
    case 52: return "4";
    case 53: return "5";
    case 54: return "6";
    case 55: return "7";
    case 56: return "8";
    case 57: return "9";
    case 59: return "SEMICOLON";
    case 61: return "EQUAL";
    case 65: return "A";
    case 66: return "B";
    case 67: return "C";
    case 68: return "D";
    case 69: return "E";
    case 70: return "F";
    case 71: return "G";
    case 72: return "H";
    case 73: return "I";
    case 74: return "J";
    case 75: return "K";
    case 76: return "L";
    case 77: return "M";
    case 78: return "N";
    case 79: return "O";
    case 80: return "P";
    case 81: return "Q";
    case 82: return "R";
    case 83: return "S";
    case 84: return "T";
    case 85: return "U";
    case 86: return "V";
    case 87: return "W";
    case 88: return "X";
    case 89: return "Y";
    case 90: return "Z";
    case 91: return "LEFT_BRACKET";
    case 92: return "BACKSLASH";
    case 93: return "RIGHT_BRACKET";
    case 96: return "GRAVE_ACCENT";
    case 161: return "WORLD_1";
    case 162: return "WORLD_2";
    case 256: return "ESCAPE";
    case 257: return "ENTER";
    case 258: return "TAB";
    case 259: return "BACKSPACE";
    case 260: return "INSERT";
    case 261: return "DELETE";
    case 262: return "RIGHT";
    case 263: return "LEFT";
    case 264: return "DOWN";
    case 265: return "UP";
    case 266: return "PAGE_UP";
    case 267: return "PAGE_DOWN";
    case 268: return "HOME";
    case 269: return "END";
    case 280: return "CAPS_LOCK";
    case 281: return "SCROLL_LOCK";
    case 282: return "NUM_LOCK";
    case 283: return "PRINT_SCREEN";
    case 284: return "PAUSE";
    case 290: return "F1";
    case 291: return "F2";
    case 292: return "F3";
    case 293: return "F4";
    case 294: return "F5";
    case 295: return "F6";
    case 296: return "F7";
    case 297: return "F8";
    case 298: return "F9";
    case 299: return "F10";
    case 300: return "F11";
    case 301: return "F12";
    case 302: return "F13";
    case 303: return "F14";
    case 304: return "F15";
    case 305: return "F16";
    case 306: return "F17";
    case 307: return "F18";
    case 308: return "F19";
    case 309: return "F20";
    case 310: return "F21";
    case 311: return "F22";
    case 312: return "F23";
    case 313: return "F24";
    case 314: return "F25";
    case 320: return "KP_0";
    case 321: return "KP_1";
    case 322: return "KP_2";
    case 323: return "KP_3";
    case 324: return "KP_4";
    case 325: return "KP_5";
    case 326: return "KP_6";
    case 327: return "KP_7";
    case 328: return "KP_8";
    case 329: return "KP_9";
    case 330: return "KP_DECIMAL";
    case 331: return "KP_DIVIDE";
    case 332: return "KP_MULTIPLY";
    case 333: return "KP_SUBTRACT";
    case 334: return "KP_ADD";
    case 335: return "KP_ENTER";
    case 336: return "KP_EQUAL";
    case 340: return "LEFT_SHIFT";
    case 341: return "LEFT_CONTROL";
    case 342: return "LEFT_ALT";
    case 343: return "LEFT_SUPER";
    case 344: return "RIGHT_SHIFT";
    case 345: return "RIGHT_CONTROL";
    case 346: return "RIGHT_ALT";
    case 347: return "RIGHT_SUPER";
    case 348: return "MENU";
    default: return "UNKNOWN";
    };
  }

  const std::vector<std::string>& get_dropped_paths() { return detail::dropped_paths; }

  void set_cursor(Input::Cursor cursor) { glfwSetCursor(detail::glfw_window, detail::cursors[(size_t)cursor]); }
}; // namespace Input
