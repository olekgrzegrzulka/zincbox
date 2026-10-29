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
#include <freetype/freetype.h>
#include <glm/mat4x4.hpp>
#include <stddef.h>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/font_face.hpp"
#include "ui/zincgui/shader.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/vertex.hpp"

namespace zincgui {
  class Widget;

  class Root final {
    public:
      Root(const Root&) = delete;
      Root(Root&&) = delete;
      Root& operator=(const Root&) = delete;
      Root& operator=(Root&&) = delete;
      Root(i32 window_width_, i32 window_height_);
      ~Root();

      template <class T, class... Args> T& add_widget(Args&&... args) {
        m_widgets_to_add.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
        T& widget = static_cast<T&>(*m_widgets_to_add.back().get());
        widget.set_window_width(m_window_width);
        widget.set_window_height(m_window_height);
        m_root_mutated = true;
        return widget;
      }

      void layout_tree(Widget*);
      [[nodiscard]] bool tick(i32 window_width_, i32 window_height_);
      void draw();

      size_t add_font(std::string_view name, const std::string& path);
      size_t add_font_from_data(std::string_view name, const void* data, size_t data_size);

      FontFace& get_font(std::string_view name);
      const FontFace& get_font(std::string_view name) const;
      FontFace& get_font(size_t id);
      const FontFace& get_font(size_t id) const;
      std::optional<size_t> find_font_id(std::string_view name) const;
      size_t get_font_count() const { return m_fonts.size(); }

      void set_default_font_size(i32 pixel_height) { m_default_font_size = pixel_height; }
      i32 get_default_font_size() const { return m_default_font_size; }

      void set_focused_widget(Widget*);
      void set_focus_manager(Widget*);

      i32 get_window_width() const { return m_window_width; }
      i32 get_window_height() const { return m_window_height; }
      vec2i get_window_size() const { return vec2i{m_window_width, m_window_height}; }
      const glm::mat4& get_matrix() const { return m_matrix; }
      const Shader& get_shader() const { return m_shader; }
      TextureAtlas& get_texture_atlas() { return m_texture_atlas; }
      std::span<Widget*> get_hovered_widgets() { return m_hovered_widgets; }
      Widget* focused_widget() const { return m_focused_widget; }
      Widget* focus_manager() const { return m_focus_manager; }

      void mark_tree_dirty() { m_tree_dirty = true; }
      void mark_layout_dirty() { m_layout_dirty = true; }

    private:
      bool process_mutations();
      void construct_traversal_order(Widget*, i32 always_on_top_layer, std::vector<std::vector<Widget*>>& layers);
      void construct_traversal_order();

    private:
      std::vector<std::vector<Vertex>> m_vertex_layers;
      size_t m_vertices_count = 0;
      u32 m_vbo = 0;
      u32 m_vao = 0;

      u64 m_current_frame = 0;
      std::unordered_set<Widget*> m_mutated_widgets;
      std::unordered_set<Widget*> m_deleted_widgets;
      Widget* m_focused_widget{};
      Widget* m_focus_manager{};
      bool m_root_mutated = true;
      bool m_tree_dirty = false;
      bool m_layout_dirty = false;
      glm::mat4 m_matrix{};
      std::vector<std::unique_ptr<Widget>> m_widgets;
      std::vector<std::unique_ptr<Widget>> m_widgets_to_add;
      std::vector<Widget*> m_widget_traversal_order;
      std::vector<Widget*> m_hovered_widgets;
      FT_Library m_freetype_lib{};
      Shader m_shader;
      std::vector<std::unique_ptr<FontFace>> m_fonts;
      std::unordered_map<std::string, size_t> m_font_name_to_id;
      i32 m_default_font_size = default_settings::default_font_size;
      TextureAtlas m_texture_atlas;
      i32 m_window_width = 0;
      i32 m_window_height = 0;
      vec2i m_mouse_pos{};
  };
} // namespace zincgui
