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
    protected:
      float nine_slice_margin =
        default_settings::sprite_nine_slice_margin; // FIXME: when this value is 0.0f, bad things happen
      float nine_slice_scale = default_settings::sprite_nine_slice_scale;
      vec2f uv_start = {0.0f, 0.0f};
      vec2f uv_end = {1.0f, 1.0f};
      i32 texture_width = default_settings::sprite_default_texture_width;
      i32 texture_height = default_settings::sprite_default_texture_height;
      u32 atlas_index = default_settings::sprite_default_atlas_index;
      std::array<Vertex, 6> vertices{};

    protected:
    public:
      Sprite(Root& ui_);
      Sprite(Root& ui_, const std::string& id_) : Widget(ui_) { set_texture(id_, true); }
      Sprite(Root& ui_, i32 width_, i32 height_);

      std::span<const Vertex> get_vertices() override;

      ~Sprite() override;

      void update() override;

      virtual zincgui::TextureAtlas& get_texture_atlas();

      void set_texture(const std::string& id, bool resize_to_texture_size = true);

      WIDGET_DEF_SETTER_DIRTY(nine_slice_margin)
      WIDGET_DEF_SETTER_DIRTY(nine_slice_scale)
      void set_uv_start(vec2f value) {
        if (!std::is_eq(uv_start.x <=> value.x) || !std::is_eq(uv_start.y <=> value.y)) {
          uv_start = value;
          mark_dirty();
        }
      }
      void set_uv_end(vec2f value) {
        if (!std::is_eq(uv_end.x <=> value.x) || !std::is_eq(uv_end.y <=> value.y)) {
          uv_end = value;
          mark_dirty();
        }
      }
      WIDGET_DEF_SETTER_DIRTY(atlas_index)

      WIDGET_DEF_GETTER(nine_slice_margin)
      WIDGET_DEF_GETTER(nine_slice_scale)
      WIDGET_DEF_GETTER(uv_start)
      WIDGET_DEF_GETTER(uv_end)
      WIDGET_DEF_GETTER(atlas_index)
  };

} // namespace zincgui
