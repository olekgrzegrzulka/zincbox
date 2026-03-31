#pragma once
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"
#include "font_face.hpp"
#include "freetype/freetype.h"
#include "label.hpp"
#include "shader.hpp"
#include "sprite.hpp"
#include "texture_atlas.hpp"
#include "widget.hpp"

class UI final {
  public:
    UI(i32 window_width_, i32 window_height_);
    ~UI() = default;

    template <class T, class... Args>
    T& add_widget(Args&&... args) {
      widgets_to_add.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
      T& widget = static_cast<T&>(*widgets_to_add.back().get());
      widget.set_window_width(window_width);
      widget.set_window_height(window_height);
      return widget;
    }

    virtual void process_input();

    virtual void update(i32 window_width_, i32 window_height_);

    virtual void draw();

    i32 get_window_width() const { return window_width; }
    i32 get_window_height() const { return window_height; }
    const glm::mat4& get_matrix() const { return matrix; }
    const FontFace& get_font_face() const { return font_face; }
    const Shader& get_sprite_shader() const { return sprite_shader; }
    const Shader& get_text_shader() const { return text_shader; }
    TextureAtlas& get_texture_atlas() { return texture_atlas; }

    void mark_dirty_recursive(Widget* w) {
      w->mark_dirty();
      for (auto& c : w->get_children()) {
        mark_dirty_recursive(c.get());
      }
    }

  private:
    void update_widget_recursive(std::unique_ptr<Widget>& widget);

    void draw_widget_recursive(Widget* widget, std::vector<Widget*>* to_be_drawn_later, std::optional<rect2i> scissor_rect = std::nullopt);

  protected:
    glm::mat4 matrix;
    std::vector<std::unique_ptr<Widget>> widgets;
    std::vector<std::unique_ptr<Widget>> widgets_to_add;
    FT_Library freetype_lib;
    Shader sprite_shader;
    FontFace font_face;
    Shader text_shader;
    TextureAtlas texture_atlas;
    i32 window_width = 0;
    i32 window_height = 0;
};
