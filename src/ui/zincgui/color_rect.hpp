#pragma once
#include <array>
#include <span>
#include "common/color.hpp"
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;

  class ColorRect : public Widget {
    public:
      ColorRect(const ColorRect&) = delete;
      ColorRect(ColorRect&&) = delete;
      ColorRect& operator=(const ColorRect&) = delete;
      ColorRect& operator=(ColorRect&&) = delete;
      ColorRect(Root& ui_);
      ColorRect(Root& ui_, i32 width_, i32 height_);
      ColorRect(Root& ui_, rgba color);
      ~ColorRect() override;

      void update() override;
      std::span<const Vertex> vertices() override;

      [[nodiscard]] vec3f color() const noexcept { return m_color; }
      template <typename Self> Self& color(this Self& self, rgba color_) {
        auto old_color = self.m_color;
        auto old_opacity = self.m_opacity;
        self.m_color.x = color_.r / 255.0f;
        self.m_color.y = color_.g / 255.0f;
        self.m_color.z = color_.b / 255.0f;
        self.m_opacity = color_.a / 255.0f;
        if (old_color != self.m_color || old_opacity != self.m_opacity) { self.mark_dirty(); }
        return self;
      }
      template <typename Self> Self& color(this Self& self, vec3f color_) {
        if (self.m_color != color_) {
          self.m_color = color_;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float opacity() const noexcept { return m_opacity; }
      template <typename Self> Self& opacity(this Self& self, float opacity_) {
        if (std::abs(self.m_opacity - opacity_) < 0.01f) { return self; }
        self.m_opacity = opacity_;
        self.mark_dirty();
        return self;
      }

    private:
      vec3f m_color = default_settings::color_rect_color;
      float m_opacity = default_settings::color_rect_opacity;
      u32 m_vbo = 0;
      u32 m_vao = 0;
      std::array<Vertex, 6> m_vertices{};
  };

} // namespace zincgui
