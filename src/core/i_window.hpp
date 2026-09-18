#pragma once
#include <string_view>
#include "common/types.hpp"
namespace zincbox {
  class IWindow {
    public:
      IWindow() = default;
      virtual ~IWindow() = default;

      IWindow(const IWindow&) = delete;
      IWindow& operator=(const IWindow&) = delete;
      IWindow(IWindow&&) = delete;
      IWindow& operator=(IWindow&&) = delete;

      virtual i32 width() const = 0;
      virtual i32 height() const = 0;
      virtual vec2i size() const = 0;
      virtual void width(i32) = 0;
      virtual void height(i32) = 0;
      virtual void resize(i32 w, i32 h) = 0;

      virtual vec2i position() const = 0;
      virtual void position(i32 x, i32 y) = 0;

      virtual void min_size(i32 w, i32 h) = 0;
      virtual void max_size(i32 w, i32 h) = 0;

      virtual std::string_view title() const = 0;
      virtual void title(std::string_view) = 0;

      virtual bool is_maximized() const = 0;
      virtual void maximize() = 0;

      virtual bool is_minimized() const = 0;
      virtual void minimize() = 0;

      virtual void restore() = 0;

      virtual bool is_fullscreen() const = 0;
      virtual void fullscreen(bool) = 0;

      virtual bool is_visible() const = 0;
      virtual void show() = 0;
      virtual void hide() = 0;

      virtual bool should_close() const = 0;
      virtual void close() = 0;

      virtual void focus() = 0;
      virtual bool has_focus() const = 0;

      virtual void set_decoration(bool) = 0;
      virtual bool is_decorated() const = 0;

      virtual void make_current() const = 0;
      virtual void swap_buffers() = 0;
      virtual void poll_events() = 0;

      virtual bool vsync() const = 0;
      virtual void vsync(bool) = 0;

      virtual void* native_handle() const = 0;

      virtual bool has_error() const = 0;
      virtual const char* get_error() const = 0;
  };
} // namespace zincbox
