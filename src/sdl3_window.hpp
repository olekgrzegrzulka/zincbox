#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <SDL3/SDL.h>
#include "common/types.hpp"
#include "core/i_window.hpp"

namespace zincbox {
  class SDL3Window final : public zincbox::IWindow {
    public:
      SDL3Window();
      SDL3Window(const char* title, i32 width, i32 height);
      ~SDL3Window() override;

      i32 width() const override;
      i32 height() const override;
      vec2i size() const override;
      void width(i32 w) override;
      void height(i32 h) override;
      void resize(i32 w, i32 h) override;

      vec2i position() const override;
      void position(i32 x, i32 y) override;

      void min_size(i32 w, i32 h) override;
      void max_size(i32 w, i32 h) override;

      std::string_view title() const override;
      void title(std::string_view t) override;

      bool is_maximized() const override;
      void maximize() override;

      bool is_minimized() const override;
      void minimize() override;

      void restore() override;

      bool is_fullscreen() const override;
      void fullscreen(bool fs) override;

      bool is_visible() const override;
      void show() override;
      void hide() override;

      bool should_close() const override;
      void close() override;

      void focus() override;
      bool has_focus() const override;

      void set_decoration(bool) override;
      bool is_decorated() const override;

      void make_current() const override;
      void swap_buffers() override;
      void poll_events() override;

      bool vsync() const override;
      void vsync(bool) override;

      void* native_handle() const override;

      virtual bool has_error() const override;
      virtual const char* get_error() const override;

    private:
      SDL_Window* m_window{};
      std::optional<const char*> m_error_message;
      bool m_should_close = false;
      std::string m_title_cache;
      SDL_GLContext m_gl_context = nullptr;
  };
} // namespace zincbox
