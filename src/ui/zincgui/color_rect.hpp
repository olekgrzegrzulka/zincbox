#pragma once
#include <array>
#include <span>
#include <vector>
#include "common/color.hpp"
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;

  class ColorRect : public Widget {

    protected:
      vec3f color = default_settings::color_rect_color;
      float opacity = default_settings::color_rect_opacity;
      u32 vbo = 0;
      u32 vao = 0;
      std::array<Vertex, 6> vertices{};

    public:
      ColorRect(const ColorRect&) = delete;
      ColorRect(ColorRect&&) = delete;
      ColorRect& operator=(const ColorRect&) = delete;
      ColorRect& operator=(ColorRect&&) = delete;
      ColorRect(Root& ui_);
      ColorRect(Root& ui_, i32 width_, i32 height_);
      ColorRect(Root& ui_, rgba color);
      ~ColorRect() override;

      std::span<const Vertex> get_vertices() override;

      void update() override;

      WIDGET_DEF_GETTER(color);

      void set_color(rgba color_) {
        auto old_color = color;
        auto old_opacity = opacity;
        color.x = color_.r / 255.0f;
        color.y = color_.g / 255.0f;
        color.z = color_.b / 255.0f;
        opacity = color_.a / 255.0f;
        if (old_color != color) { mark_dirty(); }
        if (old_opacity != opacity) { mark_dirty(); }
      }

      void set_color(vec3f color_) {
        auto old_color = color;
        color = color_;
        if (old_color != color) { mark_dirty(); }
      }

      WIDGET_DEF_GETTER(opacity)
      void set_opacity(float opacity_) {
        if (std::abs(opacity - opacity_) < 0.01f) { return; }
        opacity = opacity_;
        mark_dirty();
      }
  };

} // namespace zincgui
