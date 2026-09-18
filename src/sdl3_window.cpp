#include "sdl3_window.hpp"
#include <SDL3/SDL_video.h>
#include "common/input.hpp"
#include "common/types.hpp"
namespace zincbox {
  SDL3Window::SDL3Window() {
    m_window = SDL_CreateWindow("", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
      m_error_message = SDL_GetError();
      return;
    }
    m_gl_context = SDL_GL_CreateContext(m_window);
    if (!m_gl_context) {
      m_error_message = SDL_GetError();
      return;
    }
    m_title_cache = "";
  }

  SDL3Window::SDL3Window(const char* title, i32 width, i32 height) {
    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
      m_error_message = SDL_GetError();
      return;
    }
    m_gl_context = SDL_GL_CreateContext(m_window);
    if (!m_gl_context) {
      m_error_message = SDL_GetError();
      return;
    }
    m_title_cache = title;
  }

  SDL3Window::~SDL3Window() {
    if (m_gl_context) { SDL_GL_DestroyContext(m_gl_context); }
    if (m_window) { SDL_DestroyWindow(m_window); }
  }

  i32 SDL3Window::width() const {
    i32 w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return w;
  }

  i32 SDL3Window::height() const {
    i32 w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return h;
  }

  vec2i SDL3Window::size() const {
    i32 w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return {w, h};
  }

  void SDL3Window::width(i32 w) { SDL_SetWindowSize(m_window, w, height()); }

  void SDL3Window::height(i32 h) { SDL_SetWindowSize(m_window, width(), h); }

  void SDL3Window::resize(i32 w, i32 h) { SDL_SetWindowSize(m_window, w, h); }

  vec2i SDL3Window::position() const {
    i32 x, y;
    SDL_GetWindowPosition(m_window, &x, &y);
    return {x, y};
  }

  void SDL3Window::position(i32 x, i32 y) { SDL_SetWindowPosition(m_window, x, y); }

  void SDL3Window::min_size(i32 w, i32 h) { SDL_SetWindowMinimumSize(m_window, w, h); }

  void SDL3Window::max_size(i32 w, i32 h) { SDL_SetWindowMaximumSize(m_window, w, h); }

  std::string_view SDL3Window::title() const { return m_title_cache; }

  void SDL3Window::title(std::string_view t) {
    m_title_cache = std::string(t);
    SDL_SetWindowTitle(m_window, m_title_cache.c_str());
  }

  bool SDL3Window::is_maximized() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) != 0; }

  void SDL3Window::maximize() { SDL_MaximizeWindow(m_window); }

  bool SDL3Window::is_minimized() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) != 0; }

  void SDL3Window::minimize() { SDL_MinimizeWindow(m_window); }

  void SDL3Window::restore() { SDL_RestoreWindow(m_window); }

  bool SDL3Window::is_fullscreen() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0; }
  void SDL3Window::fullscreen(bool fs) { SDL_SetWindowFullscreen(m_window, fs); }

  bool SDL3Window::is_visible() const { return !(SDL_GetWindowFlags(m_window) & SDL_WINDOW_HIDDEN); }

  void SDL3Window::show() { SDL_ShowWindow(m_window); }
  void SDL3Window::hide() { SDL_HideWindow(m_window); }

  bool SDL3Window::should_close() const { return m_should_close; }
  void SDL3Window::close() { m_should_close = true; }

  void SDL3Window::focus() { SDL_RaiseWindow(m_window); }
  bool SDL3Window::has_focus() const { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0; }

  void SDL3Window::set_decoration(bool d) { SDL_SetWindowBordered(m_window, d); }
  bool SDL3Window::is_decorated() const { return !(SDL_GetWindowFlags(m_window) & SDL_WINDOW_BORDERLESS); }

  void SDL3Window::make_current() const { SDL_GL_MakeCurrent(m_window, m_gl_context); }

  void SDL3Window::swap_buffers() { SDL_GL_SwapWindow(m_window); }

  void SDL3Window::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) { m_should_close = true; }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_window)) {
        m_should_close = true;
      }
      Input::process_event(event);
    }
  }

  bool SDL3Window::vsync() const {
    i32 v;
    SDL_GL_GetSwapInterval(&v);
    return v;
  }

  void SDL3Window::vsync(bool v) { SDL_GL_SetSwapInterval(v); }

  void* SDL3Window::native_handle() const { return m_window; }

  bool SDL3Window::has_error() const { return m_error_message.has_value(); }

  const char* SDL3Window::get_error() const { return m_error_message.value_or(""); }
} // namespace zincbox
