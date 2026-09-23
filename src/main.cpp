
#include <csignal>

#include "common/logger.hpp"
#include "signal_handlers.hpp"
#include "zincbox.hpp"

#ifdef ZINCBOX_HAS_GUI
#include <cstddef>
#include <cstdlib>
#include <SDL3/SDL.h>
#include <nfd.hpp>
#include "opengl_includes.hpp"
#include "ui/interface.hpp"
#include "ui/sdl3_window.hpp"
#include "ui/zincgui/input.hpp"
#endif

#ifdef ZINCBOX_HAS_GUI
SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window*, const SDL_Point* p, void*) {
  zincbox::ui::DecorationHover decoration_hover = zincbox::ui::get_decoration_hover(p->x, p->y);

  switch (decoration_hover) {
  case zincbox::ui::DecorationHover::TOP_LEFT: return SDL_HITTEST_RESIZE_TOPLEFT;
  case zincbox::ui::DecorationHover::TOP_RIGHT: return SDL_HITTEST_RESIZE_TOPRIGHT;
  case zincbox::ui::DecorationHover::BOTTOM_LEFT: return SDL_HITTEST_RESIZE_BOTTOMLEFT;
  case zincbox::ui::DecorationHover::BOTTOM_RIGHT: return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
  case zincbox::ui::DecorationHover::TOP: return SDL_HITTEST_RESIZE_TOP;
  case zincbox::ui::DecorationHover::LEFT: return SDL_HITTEST_RESIZE_LEFT;
  case zincbox::ui::DecorationHover::RIGHT: return SDL_HITTEST_RESIZE_RIGHT;
  case zincbox::ui::DecorationHover::BOTTOM: return SDL_HITTEST_RESIZE_BOTTOM;
  case zincbox::ui::DecorationHover::TITLEBAR: return SDL_HITTEST_DRAGGABLE;
  case zincbox::ui::DecorationHover::INSIDE: [[__fallthrough__]];
  default: return SDL_HITTEST_NORMAL;
  }
}
#endif

int main() {
  // ----------------------------------------------------------------------
  //                Initialize libraries and signal handlers
  // ----------------------------------------------------------------------
  if (std::signal(SIGINT, handle_sigint) == SIG_ERR) { out::critical("failed to set up signal handler for SIGINT"); }
  if (std::signal(SIGTERM, handle_sigterm) == SIG_ERR) { out::critical("failed to set up signal handler for SIGTERM"); }
  if (std::signal(SIGSEGV, handle_sigsegv) == SIG_ERR) { out::critical("failed to set up signal handler for SIGSEGV"); }

#ifdef ZINCBOX_HAS_GUI
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    out::critical("failed to initialize SDL3: {}", SDL_GetError());
    exit(1);
  }
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

  if (NFD::Init() == nfdresult_t::NFD_ERROR) {
    out::critical("failed to initialize NFD: {}", NFD::GetError());
    exit(1);
  }
#endif

  // ----------------------------------------------------------------------
  //               Initialize zincbox submodules, deserialize
  // ----------------------------------------------------------------------
  zincbox::load_state_from_json();
  zincbox::init(zincbox::InitFlags::WINDOW | zincbox::InitFlags::MPRIS);
#ifdef ZINCBOX_HAS_GUI
  SDL_Window* sdl_window = zincbox::window()->native_handle();
  SDL_SetWindowHitTest(sdl_window, hit_test_callback, NULL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  zincgui::Input::init(sdl_window);
#endif
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
#ifdef ZINCBOX_HAS_GUI
  NFD_Quit();
  SDL_Quit();
#endif
}
