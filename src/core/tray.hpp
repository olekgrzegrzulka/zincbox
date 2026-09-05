#pragma once
#include <SDL3/SDL.h>

namespace tray {
  void init(SDL_Window*);
  void update();
  void deinit();
} // namespace tray
