#pragma once
#include <vector>
#include "common/color.hpp"
#include "common/types.hpp"
#include "widget.hpp"

class UI;

class ColorRect : public Widget {
    struct vertex final {
        i32 type = 2;
        vec2f pos;
        vec3f color{1.0f};
        float opacity{1.0f};

        vertex(vec2f pos_, vec3f color_, float opacity_ = 1.0f) {
          pos = pos_;
          color = color_;
          opacity = opacity_;
        }
    };

  protected:
    vec3f color{};
    float opacity = 1.0f;
    u32 vbo = 0;
    u32 vao = 0;
    std::vector<ColorRect::vertex> vertices{};

  public:
    ColorRect(UI& ui_);
    ColorRect(UI& ui_, i32 width_, i32 height_);
    ColorRect(UI& ui_, rgba color);
    ~ColorRect() override;

    void update() override;
    void draw() override;

    void update_mesh();
    void setup_buffers();

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
    WIDGET_DEF_SETTER_DIRTY(opacity)
};
