#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "common/color.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;

  enum class TextAlign : u8 { LEFT, CENTER, RIGHT, JUSTIFY };

  struct vertex_label final {
      i32 type = 1;
      vec2f pos;
      vec2f uv;
      vec3f color{1.0f};
  };

  class Label final : public Widget {
    private:
      struct LocalQuad {
          vec2f pos_min{};
          vec2f pos_max{};
          vec2f uv_min{};
          vec2f uv_max{};
          vec3f color{1.0f};
      };

      std::string text{};
      size_t font_index = 0;
      i32 font_size = 0;
      float line_spacing = default_settings::label_line_spacing;
      bool resize_to_text_extents = default_settings::label_resize_to_text_extents;
      bool parse_tags = default_settings::label_parse_tags;
      bool word_wrap = default_settings::label_word_wrap;
      bool ellipsis = default_settings::label_ellipsis;
      bool custom_alignment_set = false;
      TextAlign text_alignment = TextAlign::LEFT;
      Anchor label_anchor = Anchor::CENTER;

      u32 vao = 0;
      u32 vbo = 0;
      size_t vbo_capacity = 0;
      std::vector<LocalQuad> local_quads{};
      std::vector<vertex_label> vertices{};
      vec3f text_color = {1.0f, 1.0f, 1.0f};
      vec2f text_extents{};

      bool shape_dirty = true;
      bool vertices_dirty = true;
      vec2i last_layout_size{-1, -1};
      vec2i last_screen_pos{-999999, -999999};
      vec2i last_window_size{-1, -1};
      u64 last_atlas_version = 0;

    public:
      Label(const Label&) = delete;
      Label(Label&&) = delete;
      Label& operator=(const Label&) = delete;
      Label& operator=(Label&&) = delete;
      Label(Root&);
      Label(Root&, std::string_view);
      ~Label() override;

      void set_resize_to_text_extents(bool to);
      WIDGET_DEF_GETTER(resize_to_text_extents);
      WIDGET_DEF_GETTER(text);

      vec2i get_text_extents() {
        if (shape_dirty) { update_text(); }
        return text_extents;
      }

      void set_label_anchor(Anchor a) {
        if (label_anchor == a) { return; }
        label_anchor = a;
        shape_dirty = true;
        mark_dirty();
      }
      Anchor get_label_anchor() const { return label_anchor; }

      void set_text_alignment(TextAlign align) {
        if (custom_alignment_set && text_alignment == align) { return; }
        custom_alignment_set = true;
        text_alignment = align;
        shape_dirty = true;
        mark_dirty();
      }
      TextAlign get_text_alignment() const { return text_alignment; }

      void set_font(size_t index);
      void set_font(std::string_view name);
      size_t get_font_index() const { return font_index; }

      void set_font_size(i32 size) {
        if (font_size == size) { return; }
        font_size = size;
        shape_dirty = true;
        mark_dirty();
      }
      i32 get_font_size() const;

      void set_parse_tags(bool enable) {
        if (parse_tags == enable) { return; }
        parse_tags = enable;
        shape_dirty = true;
        mark_dirty();
      }
      bool get_parse_tags() const { return parse_tags; }

      void set_word_wrap(bool wrap) {
        if (word_wrap == wrap) { return; }
        word_wrap = wrap;
        shape_dirty = true;
        mark_dirty();
      }
      bool get_word_wrap() const { return word_wrap; }

      void set_ellipsis(bool enable) {
        if (ellipsis == enable) { return; }
        ellipsis = enable;
        shape_dirty = true;
        mark_dirty();
      }
      bool get_ellipsis() const { return ellipsis; }

      void set_line_spacing(float spacing) {
        if (line_spacing == spacing) { return; }
        line_spacing = spacing;
        shape_dirty = true;
        mark_dirty();
      }
      float get_line_spacing() const { return line_spacing; }

      void set_text(std::string_view text_) {
        if (text == text_) { return; }
        text = text_;
        shape_dirty = true;
        mark_dirty();
      }

      vec3f get_text_color() const { return text_color; }

      void set_text_color(rgba text_color_) {
        set_text_color(vec3f{text_color_.r / 255.0f, text_color_.g / 255.0f, text_color_.b / 255.0f});
      }

      void set_text_color(glm::vec3 text_color_) {
        if (text_color == text_color_) { return; }
        text_color = text_color_;
        shape_dirty = true;
        mark_dirty();
      }

      void append_text(std::string_view append) {
        if (append.empty()) { return; }
        text += append;
        shape_dirty = true;
        mark_dirty();
      }

      bool erase_last_character() {
        if (text.empty()) { return false; }
        auto it = text.end();
        utf8::prior(it, text.begin());
        text.erase(it, text.end());
        shape_dirty = true;
        mark_dirty();
        return true;
      }

      void update() override;
      void draw() override;

    protected:
      void update_text();
      void rebuild_screen_vertices();
      void setup_buffers();
  };

} // namespace zincgui
