
#include <csignal>

#include "common/logger.hpp"
#include "signal_handlers.hpp"
#include "zincbox.hpp"

#ifdef ZINCBOX_HAS_GUI
#include <cstdlib>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <nfd.hpp>
#include "ui/sdl3_window.hpp"
#include "ui/zincgui/input.hpp"
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
  zincbox::load_db_from_file();
  zincbox::load_state_from_json();
  zincbox::init(zincbox::InitFlags::WINDOW | zincbox::InitFlags::MPRIS);
#ifdef ZINCBOX_HAS_GUI
  SDL_Window* sdl_window = zincbox::window()->native_handle();
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
