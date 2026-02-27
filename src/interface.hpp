#pragma once
#include "types.hpp"

class PopupController;

namespace interface {
  void init();
  void deinit();
  void process_input();
  void update(vec2i window_size);
  void draw();

  PopupController* get_popup_controller();
}; // namespace interface
