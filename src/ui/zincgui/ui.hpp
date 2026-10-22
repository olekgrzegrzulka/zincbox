#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <glm/mat4x4.hpp>
#include <stddef.h>
#include "common/types.hpp"
#include "font_face.hpp"
#include "freetype/freetype.h"
#include "shader.hpp"
#include "texture_atlas.hpp"

namespace zincgui {
  class Widget;

  class Root final {
    public:
      Root(i32 window_width_, i32 window_height_);
      ~Root();

      template <class T, class... Args> T& add_widget(Args&&... args) {
        widgets_to_add.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
        T& widget = static_cast<T&>(*widgets_to_add.back().get());
        widget.set_window_width(window_width);
        widget.set_window_height(window_height);
        root_mutated = true;
        return widget;
      }

      void tick(i32 window_width_, i32 window_height_);
      void draw();

      size_t add_font(std::string_view name, const std::string& path);
      size_t add_font_from_data(std::string_view name, const void* data, size_t data_size);

      FontFace& get_font(std::string_view name);
      const FontFace& get_font(std::string_view name) const;
      FontFace& get_font(size_t id);
      const FontFace& get_font(size_t id) const;
      std::optional<size_t> find_font_id(std::string_view name) const;
      size_t get_font_count() const { return fonts.size(); }

      void set_default_font_size(i32 pixel_height) { default_font_size = pixel_height; }
      i32 get_default_font_size() const { return default_font_size; }

      i32 get_window_width() const { return window_width; }
      i32 get_window_height() const { return window_height; }
      vec2i get_window_size() const { return vec2i{window_width, window_height}; }
      const glm::mat4& get_matrix() const { return matrix; }
      const Shader& get_shader() const { return shader; }
      TextureAtlas& get_texture_atlas() { return texture_atlas; }
      std::span<Widget*> get_hovered_widgets() { return hovered_widgets; }

      void mark_tree_dirty() { tree_dirty = true; }

    protected:
      u64 current_frame = 0;
      void update_hovered_widgets_recursive(Widget* widget);
      bool process_mutations();
      void construct_traversal_order(Widget*, i32 always_on_top_layer = 0);
      void construct_traversal_order();

    protected:
      std::unordered_set<Widget*> mutated_widgets;
      std::unordered_set<Widget*> deleted_widgets;
      bool root_mutated = true;
      bool tree_dirty = false;
      glm::mat4 matrix;
      std::vector<std::unique_ptr<Widget>> widgets;
      std::vector<std::unique_ptr<Widget>> widgets_to_add;
      std::vector<std::vector<Widget*>> widget_traversal_order;
      std::vector<Widget*> hovered_widgets;
      FT_Library freetype_lib{};
      Shader shader;
      std::vector<std::unique_ptr<FontFace>> fonts;
      std::unordered_map<std::string, size_t> font_name_to_id;
      i32 default_font_size = 14;
      TextureAtlas texture_atlas;
      i32 window_width = 0;
      i32 window_height = 0;
  };
} // namespace zincgui
