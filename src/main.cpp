#include "opengl_includes.hpp"

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <SDL3/SDL.h>
#include <nfd.hpp>
#include "common/input.hpp"
#include "common/logger.hpp"
#include "core/zincbox.hpp"
#include "sdl3_window.hpp"
#include "signal_handlers.hpp"
#include "ui/interface.hpp"

SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window*, const SDL_Point* p, void*) {
  interface::DecorationHover decoration_hover = interface::get_decoration_hover(p->x, p->y);

  switch (decoration_hover) {
  case interface::DecorationHover::TOP_LEFT: return SDL_HITTEST_RESIZE_TOPLEFT;
  case interface::DecorationHover::TOP_RIGHT: return SDL_HITTEST_RESIZE_TOPRIGHT;
  case interface::DecorationHover::BOTTOM_LEFT: return SDL_HITTEST_RESIZE_BOTTOMLEFT;
  case interface::DecorationHover::BOTTOM_RIGHT: return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
  case interface::DecorationHover::TOP: return SDL_HITTEST_RESIZE_TOP;
  case interface::DecorationHover::LEFT: return SDL_HITTEST_RESIZE_LEFT;
  case interface::DecorationHover::RIGHT: return SDL_HITTEST_RESIZE_RIGHT;
  case interface::DecorationHover::BOTTOM: return SDL_HITTEST_RESIZE_BOTTOM;
  case interface::DecorationHover::TITLEBAR: return SDL_HITTEST_DRAGGABLE;
  case interface::DecorationHover::INSIDE: [[__fallthrough__]];
  default: return SDL_HITTEST_NORMAL;
  }
}

int main() {
  // ----------------------------------------------------------------------
  //                Initialize libraries and signal handlers
  // ----------------------------------------------------------------------
  if (std::signal(SIGINT, handle_sigint) == SIG_ERR) { out::critical("failed to set up signal handler for SIGINT"); }
  if (std::signal(SIGTERM, handle_sigterm) == SIG_ERR) { out::critical("failed to set up signal handler for SIGTERM"); }
  if (std::signal(SIGSEGV, handle_sigsegv) == SIG_ERR) { out::critical("failed to set up signal handler for SIGSEGV"); }

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    out::critical("failed to initialize SDL3: {}", SDL_GetError());
    exit(1);
  }
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

  if (NFD::Init() == nfdresult_t::NFD_ERROR) {
    out::critical("failed to initialize NFD: {}", NFD::GetError());
    exit(1);
  }

  // ----------------------------------------------------------------------
  //               Initialize zincbox submodules, deserialize
  // ----------------------------------------------------------------------
  zincbox::load_state_from_json();
  zincbox::init<zincbox::SDL3Window>(zincbox::InitFlags::WINDOW | zincbox::InitFlags::MPRIS);
  SDL_Window* sdl_window = static_cast<SDL_Window*>(zincbox::window()->native_handle());
  SDL_SetWindowHitTest(sdl_window, hit_test_callback, NULL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  Input::init(sdl_window);
  zincbox::apply_loaded_state();

  // ----------------------------------------------------------------------
  //                               Main loop
  // ----------------------------------------------------------------------
  zincbox::run();

  // ----------------------------------------------------------------------
  //                      Serialize, de-init, clean-up
  // ----------------------------------------------------------------------
  zincbox::save_state_to_json();
  zincbox::save_db_to_file();
  zincbox::deinit();
  NFD_Quit();
  SDL_Quit();
}
