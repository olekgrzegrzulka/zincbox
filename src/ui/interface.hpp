#pragma once
#include "common/types.hpp"
#include "lib/json.cpp/json.h"

namespace interface {
  void init();
  void deinit();
  void update(vec2i window_size);

  enum class DecorationHover : u8 {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP,
    LEFT,
    RIGHT,
    BOTTOM,
    INSIDE,
    TITLEBAR,
  };

  DecorationHover get_decoration_hover();

  void on_minimize_button_pressed(std::function<void()>);
  void on_maximize_button_pressed(std::function<void()>);
  void on_close_button_pressed(std::function<void()>);

  jt::Json to_json();
  void from_json(const jt::Json&);
}; // namespace interface
