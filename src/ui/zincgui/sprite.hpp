#pragma once
#include <span>
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class TextureAtlas;
  class Root;
  class Shader;

  class Sprite : public Widget {
    public:
      Sprite(Root& ui_);
      Sprite(Root& ui_, const std::string& id_);
      Sprite(Root& ui_, i32 width_, i32 height_);
      ~Sprite() override;

      void update() override;
      std::span<const Vertex> vertices() override;

      virtual zincgui::TextureAtlas& texture_atlas();

      [[nodiscard]] float nine_slice_margin() const noexcept { return m_nine_slice_margin; }
      template <typename Self> Self& nine_slice_margin(this Self& self, float value) {
        if (self.m_nine_slice_margin != value) {
          self.m_nine_slice_margin = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float nine_slice_scale() const noexcept { return m_nine_slice_scale; }
      template <typename Self> Self& nine_slice_scale(this Self& self, float value) {
        if (self.m_nine_slice_scale != value) {
          self.m_nine_slice_scale = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] vec2f uv_start() const noexcept { return m_uv_start; }
      template <typename Self> Self& uv_start(this Self& self, vec2f value) {
        if (!std::is_eq(self.m_uv_start.x <=> value.x) || !std::is_eq(self.m_uv_start.y <=> value.y)) {
          self.m_uv_start = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] vec2f uv_end() const noexcept { return m_uv_end; }
      template <typename Self> Self& uv_end(this Self& self, vec2f value) {
        if (!std::is_eq(self.m_uv_end.x <=> value.x) || !std::is_eq(self.m_uv_end.y <=> value.y)) {
          self.m_uv_end = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] u32 atlas_index() const noexcept { return m_atlas_index; }
      template <typename Self> Self& atlas_index(this Self& self, u32 value) {
        if (self.m_atlas_index != value) {
          self.m_atlas_index = value;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 texture_width() const noexcept { return m_texture_width; }
      template <typename Self> Self& texture_width(this Self& self, i32 value) {
        self.m_texture_width = value;
        return self;
      }

      [[nodiscard]] i32 texture_height() const noexcept { return m_texture_height; }
      template <typename Self> Self& texture_height(this Self& self, i32 value) {
        self.m_texture_height = value;
        return self;
      }

      template <typename Self>
      Self& texture(this Self& self, const std::string& id, bool resize_to_texture_size = true) {
        self.set_texture_impl(id, resize_to_texture_size);
        return self;
      }

    private:
      void set_texture_impl(const std::string& id, bool resize_to_texture_size);

    private:
      float m_nine_slice_margin = default_settings::sprite_nine_slice_margin;
      float m_nine_slice_scale = default_settings::sprite_nine_slice_scale;
      vec2f m_uv_start = {0.0f, 0.0f};
      vec2f m_uv_end = {1.0f, 1.0f};
      i32 m_texture_width = default_settings::sprite_default_texture_width;
      i32 m_texture_height = default_settings::sprite_default_texture_height;
      u32 m_atlas_index = default_settings::sprite_default_atlas_index;
      std::array<Vertex, 6> m_vertices{};
  };

} // namespace zincgui
