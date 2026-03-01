#pragma once
#include <functional>
#include <vector>
#include <glm/vec2.hpp>
#include "../types.hpp"
#include "widget.hpp"

class TextureAtlas;
class UI;
class Shader;

struct vertex_sprite {
    vec2f pos;
    vec2f uv;
    float nine_slice_margin = 0.0f;
    float nine_slice_scale = 1.0f;
    glm::vec<2, u32> widget_size = {1.0f, 1.0f};
    vec2f uv_start = {0.0f, 0.0f};
    vec2f uv_end = {1.0f, 1.0f};
};

class Sprite : public Widget {
  protected:
    float nine_slice_margin = 1.0f; // FIXME: when this value is 0.0f, bad things happen
    float nine_slice_scale = 1.0f;
    vec2f uv_start = {0.0f, 0.0f};
    vec2f uv_end = {1.0f, 1.0f};
    i32 texture_width = 16;
    i32 texture_height = 16;

  protected:
    u32 vbo = 0;
    u32 vao = 0;
    std::vector<vertex_sprite> vertices{};
    std::optional<std::reference_wrapper<const Shader>> shader{};

  public:
    Sprite(UI& ui_);
    Sprite(UI& ui_, std::string id_) : Widget(ui_) {
      set_texture(id_, true);
    }
    Sprite(UI& ui_, i32 width_, i32 height_);

    ~Sprite() override;

    void update() override;

    void draw() override;

    void update_mesh();

    void setup_buffers();

    virtual TextureAtlas& get_texture_atlas();

    void set_texture(std::string id, bool resize_to_texture_size = true);

    void set_shader(const Shader& shader_) {
      shader = std::reference_wrapper{shader_};
      uv_start = {0.0f, 0.0f};
      uv_end = {1.0f, 1.0f};
      mark_dirty();
    }

    WIDGET_DEF_SETTER_DIRTY(nine_slice_margin)
    WIDGET_DEF_SETTER_DIRTY(nine_slice_scale)
    WIDGET_DEF_SETTER_DIRTY(uv_start)
    WIDGET_DEF_SETTER_DIRTY(uv_end)

    WIDGET_DEF_GETTER(nine_slice_margin)
    WIDGET_DEF_GETTER(nine_slice_scale)
    WIDGET_DEF_GETTER(uv_start)
    WIDGET_DEF_GETTER(uv_end)
};