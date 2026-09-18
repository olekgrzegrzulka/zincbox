#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "common/color.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "widget.hpp"

class UI;

struct vertex_label final {
    i32 type = 1;
    vec2f pos;
    vec2f uv;
    vec3f color{1.0};
};

class Label final : public Widget {
  private:
    std::string text;
    bool resize_to_text_extents = true;
    Anchor label_anchor = Anchor::CENTER;
    u32 vao = 0;
    u32 vbo = 0;
    std::vector<vertex_label> vertices;
    vec3f text_color = {1.0f, 1.0f, 1.0f};
    vec2f text_extents{};

  public:
    Label(UI&);
    Label(UI&, std::string_view);

    ~Label() override;

    // WIDGET_DEF_GETTER(text_length);
    void set_resize_to_text_extents(bool to);
    WIDGET_DEF_GETTER(resize_to_text_extents);
    WIDGET_DEF_GETTER(text);
    WIDGET_DEF_GETTER(text_extents);
    WIDGET_DEF_SETTER_DIRTY(label_anchor);

    void set_text(std::string_view text_) {
      if (text == text_) { return; }
      text = text_;
      // text_dirty = true;
      dirty = true;
    }

    vec3f get_text_color() const { return text_color; }

    void set_text_color(rgba text_color_) {
      set_text_color(vec3f{text_color_.r / 255.0, text_color_.g / 255.0, text_color_.b / 255.0});
    }

    void set_text_color(glm::vec3 text_color_) {
      if (text_color == text_color_) { return; }
      text_color = text_color_;
      // text_dirty = true;
      dirty = true;
    }

    void append_text(std::string_view append) {
      if (append.empty()) { return; }
      text += append;
      // text_dirty = true;
      dirty = true;
    }

    bool erase_last_character() {
      if (text.empty()) { return false; }

      auto it = text.end();
      utf8::prior(it, text.begin());
      text.erase(it, text.end());

      dirty = true;
      return true;
    }

    void update() override;

    void draw() override;

  protected:
    void update_mesh();

    void setup_buffers();
};
