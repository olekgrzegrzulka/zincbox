#pragma once
#include "common/types.hpp"
#include "lib/json.cpp/json.h"

namespace interface {
  void init();
  void deinit();
  void input();
  void update(vec2i window_size);
  void draw();

  jt::Json to_json();
  void from_json(const jt::Json&);
}; // namespace interface
