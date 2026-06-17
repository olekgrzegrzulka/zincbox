#pragma once
#include "common/types.hpp"

class PopupController;

namespace interface {
  void init();
  void deinit();
  void input();
  void update(vec2i window_size);
  void draw();

  PopupController* get_popup_controller();
}; // namespace interface
