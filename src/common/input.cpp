#include "input.hpp"
#include <algorithm>
#include <array>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include "common/types.hpp"
#include "common/utf.hpp"

namespace Input {

  namespace detail {
    static SDL_Window* sdl_window{};
    static i32 mouse_x{};
    static i32 mouse_y{};
    static i32 last_mouse_x{};
    static i32 last_mouse_y{};
    static i32 window_x{};
    static i32 window_y{};
    static std::vector<InputEvent> event_queue{};
    static std::vector<std::string> dropped_paths{};
    static std::array<SDL_Cursor*, (size_t)Cursor::CURSOR_SIZE> cursors{};

    enum class ButtonState : u8 { RELEASED, PRESSED, JUST_RELEASED, JUST_PRESSED };

    std::array<ButtonState, (size_t)MouseButton::MOUSE_BUTTON_SIZE> mouse_states;
    std::array<ButtonState, (size_t)Key::KEY_SIZE> key_states;
    std::u32string keyboard_characters_prev;
    std::u32string keyboard_characters_curr;
    vec2f accumulated_scroll_next{};
    vec2f accumulated_scroll{};
  } // namespace detail

  void process_event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION: {
      InputEventMouseMove ev{.to = {static_cast<double>(event.motion.x), static_cast<double>(event.motion.y)}};
      detail::event_queue.emplace_back(ev);
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: [[fallthrough]];
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      InputEventMouseButton ev = {
        .button = static_cast<MouseButton>(event.button.button),
        .action = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? MouseAction::PRESS : MouseAction::RELEASE,
      };
      detail::event_queue.emplace_back(ev);
      break;
    }
    case SDL_EVENT_DROP_FILE: {
      detail::dropped_paths.emplace_back(event.drop.data);
      break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
      detail::accumulated_scroll_next += vec2f{event.wheel.x, event.wheel.y};
      InputEventMouseScroll ev = {
        .offset = {static_cast<double>(event.wheel.x), static_cast<double>(event.wheel.y)},
      };
      detail::event_queue.emplace_back(ev);
      break;
    }
    case SDL_EVENT_TEXT_INPUT: {
      std::string utf8_text = event.text.text;
      std::u32string utf32_text = utf8_to_utf32(utf8_text);
      detail::keyboard_characters_curr += utf32_text;
      break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      KeyAction key_action = KeyAction::RELEASE;
      if (event.type == SDL_EVENT_KEY_DOWN) { key_action = event.key.repeat ? KeyAction::REPEAT : KeyAction::PRESS; }
      InputEventKey ev = {
        .key = static_cast<Key>(event.key.key),
        .action = key_action,
        .scancode = static_cast<i32>(event.key.scancode),
      };
      detail::event_queue.emplace_back(ev);
      break;
    }
    case SDL_EVENT_WINDOW_MOUSE_ENTER: detail::event_queue.emplace_back(InputEventMouseEntered{}); break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE: detail::event_queue.emplace_back(InputEventMouseLeft{}); break;
    default: break;
    }
  }

  void init(SDL_Window* window) {
    detail::sdl_window = window;
    SDL_StartTextInput(window);

    detail::cursors[(size_t)Cursor::ARROW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    detail::cursors[(size_t)Cursor::IBEAM] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    detail::cursors[(size_t)Cursor::CROSSHAIR] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    detail::cursors[(size_t)Cursor::HAND] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    detail::cursors[(size_t)Cursor::RESIZE_HORIZONTAL] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    detail::cursors[(size_t)Cursor::RESIZE_VERTICAL] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    detail::cursors[(size_t)Cursor::RESIZE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    detail::cursors[(size_t)Cursor::NOT_ALLOWED] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
  }

  void update() {
    float x = 0.0f;
    float y = 0.0f;
    SDL_GetWindowPosition(detail::sdl_window, &detail::window_x, &detail::window_y);
    SDL_MouseButtonFlags mouse_mask = SDL_GetMouseState(&x, &y);
    detail::last_mouse_x = detail::mouse_x;
    detail::last_mouse_y = detail::mouse_y;
    detail::mouse_x = static_cast<i32>(x);
    detail::mouse_y = static_cast<i32>(y);

    for (size_t i = 0; i < detail::mouse_states.size(); i += 1) {
      using enum detail::ButtonState;
      bool is_pressed = (mouse_mask & SDL_BUTTON_MASK(i)) != 0;

      if (detail::mouse_states[i] == RELEASED && is_pressed) {
        detail::mouse_states[i] = JUST_PRESSED;
      } else if (detail::mouse_states[i] == PRESSED && !is_pressed) {
        detail::mouse_states[i] = JUST_RELEASED;
      } else if (detail::mouse_states[i] == JUST_RELEASED) {
        detail::mouse_states[i] = is_pressed ? JUST_PRESSED : RELEASED;
      } else if (detail::mouse_states[i] == JUST_PRESSED) {
        detail::mouse_states[i] = is_pressed ? PRESSED : JUST_RELEASED;
      }
    }

    i32 num_keys = 0;
    const bool* keyboard_state = SDL_GetKeyboardState(&num_keys);
    for (size_t scancode = 0; scancode < detail::key_states.size(); scancode += 1) {
      using enum detail::ButtonState;
      bool is_pressed = keyboard_state[scancode];

      if (detail::key_states[scancode] == RELEASED && is_pressed) {
        detail::key_states[scancode] = JUST_PRESSED;
      } else if (detail::key_states[scancode] == PRESSED && !is_pressed) {
        detail::key_states[scancode] = JUST_RELEASED;
      } else if (detail::key_states[scancode] == JUST_RELEASED) {
        detail::key_states[scancode] = is_pressed ? JUST_PRESSED : RELEASED;
      } else if (detail::key_states[scancode] == JUST_PRESSED) {
        detail::key_states[scancode] = is_pressed ? PRESSED : JUST_RELEASED;
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

  bool ctrl_pressed() { return key_pressed(Key::KEY_LEFT_CONTROL) || key_pressed(Key::KEY_RIGHT_CONTROL); }

  bool shift_pressed() { return key_pressed(Key::KEY_LEFT_SHIFT) || key_pressed(Key::KEY_RIGHT_SHIFT); }

  vec2f get_mouse_scroll() { return detail::accumulated_scroll; }

  vec2i get_mouse_delta() { return {detail::mouse_x - detail::last_mouse_x, detail::mouse_y - detail::last_mouse_y}; }

  vec2i get_window_size() {
    i32 width = 0;
    i32 height = 0;
    SDL_GetWindowSize(detail::sdl_window, &width, &height);
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

  void reset_cursor() { SDL_SetCursor(SDL_GetDefaultCursor()); }
  void set_cursor(Input::Cursor cursor) { SDL_SetCursor(detail::cursors[(size_t)cursor]); }
}; // namespace Input
