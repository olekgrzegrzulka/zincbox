#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <SDL3/SDL.h>
#include "common/types.hpp"

namespace zincbox {
  class SDL3Window final {
    public:
      SDL3Window();
      SDL3Window(const char* title, i32 width, i32 height);
      ~SDL3Window();

      i32 width() const;
      i32 height() const;
      vec2i size() const;
      void width(i32 w);
      void height(i32 h);
      void resize(i32 w, i32 h);

      vec2i position() const;
      void position(i32 x, i32 y);

      void min_size(i32 w, i32 h);
      void max_size(i32 w, i32 h);

      std::string_view title() const;
      void title(std::string_view t);

      bool is_maximized() const;
      void maximize();

      bool is_minimized() const;
      void minimize();

      void restore();

      bool is_fullscreen() const;
      void fullscreen(bool fs);

      bool is_visible() const;
      void show();
      void hide();

      bool should_close() const;
      void close();

      void focus();
      bool has_focus() const;

      void set_decoration(bool);
      bool is_decorated() const;

      void make_current() const;
      void swap_buffers();
      void poll_events();

      bool vsync() const;
      void vsync(bool);

      SDL_Window* native_handle() const;

      bool has_error() const;
      const char* get_error() const;

    private:
      SDL_Window* m_window{};
      std::optional<const char*> m_error_message;
      bool m_should_close = false;
      std::string m_title_cache;
      SDL_GLContext m_gl_context = nullptr;
  };
} // namespace zincbox
