#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include "common/types.hpp"
#include "widget.hpp"

class TextureAtlas;
class UI;
class Shader;

class ColorRect : public Widget {
    struct vertex final {
        i32 type = 2;
        vec2f pos;
        vec3f color{1.0};

        vertex(vec2f pos_, vec3f color_) {
          pos = pos_;
          color = color_;
        }
    };

  protected:
    vec3f color{};
    u32 vbo = 0;
    u32 vao = 0;
    std::vector<ColorRect::vertex> vertices{};

  public:
    ColorRect(UI& ui_);
    ColorRect(UI& ui_, i32 width_, i32 height_);
    ~ColorRect() override;

    void update() override;
    void draw() override;

    void update_mesh();
    void setup_buffers();

    WIDGET_DEF_GETTER(color);
    WIDGET_DEF_SETTER_DIRTY(color);
};
