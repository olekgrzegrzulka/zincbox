#pragma once
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "common/color.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;

  enum class TextAlign : u8 { LEFT, CENTER, RIGHT, JUSTIFY };

  class Label final : public Widget {
    public:
      Label(Root&);
      Label(Root&, std::string_view);
      ~Label() override;

      void update() override;
      std::span<const Vertex> vertices() override;

      void append_text(std::string_view append) {
        if (append.empty()) { return; }
        m_text += append;
        m_shape_dirty = true;
        mark_dirty();
      }

      bool erase_last_character() {
        if (m_text.empty()) { return false; }
        auto it = m_text.end();
        utf8::prior(it, m_text.begin());
        m_text.erase(it, m_text.end());
        m_shape_dirty = true;
        mark_dirty();
        return true;
      }

      void font(size_t index);
      void font(std::string_view name);
      [[nodiscard]] size_t font_index() const noexcept { return m_font_index; }

      [[nodiscard]] i32 font_size() const;
      template <typename Self> Self& font_size(this Self& self, i32 size) {
        if (self.m_font_size == size) { return self; }
        self.m_font_size = size;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] bool resize_to_text_extents() const noexcept { return m_resize_to_text_extents; }
      Label& resize_to_text_extents(bool to);

      [[nodiscard]] const std::string& text() const noexcept { return m_text; }
      template <typename Self> Self& text(this Self& self, std::string_view text_) {
        if (self.m_text == text_) { return self; }
        self.m_text = text_;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] vec2i text_extents() {
        if (m_shape_dirty) { update_text(); }
        return m_text_extents;
      }

      [[nodiscard]] Anchor label_anchor() const noexcept { return m_label_anchor; }
      template <typename Self> Self& label_anchor(this Self& self, Anchor a) {
        if (self.m_label_anchor == a) { return self; }
        self.m_label_anchor = a;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] TextAlign text_alignment() const noexcept { return m_text_alignment; }
      template <typename Self> Self& text_alignment(this Self& self, TextAlign align) {
        if (self.m_custom_alignment_set && self.m_text_alignment == align) { return self; }
        self.m_custom_alignment_set = true;
        self.m_text_alignment = align;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] bool parse_tags() const noexcept { return m_parse_tags; }
      template <typename Self> Self& parse_tags(this Self& self, bool enable) {
        if (self.m_parse_tags == enable) { return self; }
        self.m_parse_tags = enable;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] bool word_wrap() const noexcept { return m_word_wrap; }
      template <typename Self> Self& word_wrap(this Self& self, bool wrap) {
        if (self.m_word_wrap == wrap) { return self; }
        self.m_word_wrap = wrap;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] bool ellipsis() const noexcept { return m_ellipsis; }
      template <typename Self> Self& ellipsis(this Self& self, bool enable) {
        if (self.m_ellipsis == enable) { return self; }
        self.m_ellipsis = enable;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] float line_spacing() const noexcept { return m_line_spacing; }
      template <typename Self> Self& line_spacing(this Self& self, float spacing) {
        if (self.m_line_spacing == spacing) { return self; }
        self.m_line_spacing = spacing;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

      [[nodiscard]] vec3f text_color() const noexcept { return m_text_color; }
      template <typename Self> Self& text_color(this Self& self, rgba text_color_) {
        return self.text_color(vec3f{text_color_.r / 255.0f, text_color_.g / 255.0f, text_color_.b / 255.0f});
      }
      template <typename Self> Self& text_color(this Self& self, glm::vec3 text_color_) {
        if (self.m_text_color == text_color_) { return self; }
        self.m_text_color = text_color_;
        self.m_shape_dirty = true;
        self.mark_dirty();
        return self;
      }

    protected:
      void update_text();
      void rebuild_screen_vertices();

    private:
      struct LocalQuad {
          vec2f pos_min{};
          vec2f pos_max{};
          vec2f uv_min{};
          vec2f uv_max{};
          vec3f color{1.0f};
      };

      std::string m_text;
      size_t m_font_index = 0;
      i32 m_font_size = 0;
      float m_line_spacing = default_settings::label_line_spacing;
      bool m_resize_to_text_extents = default_settings::label_resize_to_text_extents;
      bool m_parse_tags = default_settings::label_parse_tags;
      bool m_word_wrap = default_settings::label_word_wrap;
      bool m_ellipsis = default_settings::label_ellipsis;
      bool m_custom_alignment_set = false;
      TextAlign m_text_alignment = TextAlign::LEFT;
      Anchor m_label_anchor = Anchor::CENTER;

      u32 m_vao = 0;
      u32 m_vbo = 0;
      size_t m_vbo_capacity = 0;
      std::vector<LocalQuad> m_local_quads{};
      std::vector<Vertex> m_vertices;
      vec3f m_text_color = {1.0f, 1.0f, 1.0f};
      vec2f m_text_extents{};

      bool m_shape_dirty = true;
      bool m_vertices_dirty = true;
      vec2i m_last_layout_size{-1, -1};
      vec2i m_last_screen_pos{-999999, -999999};
      vec2i m_last_window_size{-1, -1};
      u64 m_last_atlas_version = 0;
  };

} // namespace zincgui
