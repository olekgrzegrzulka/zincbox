#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <glm/vec3.hpp>
#include "../types.hpp"
#include "../utf.hpp"
#include "../vertex.hpp"
#include "widget.hpp"

class UI;
class Sprite;

class Label final : public Widget {
  private:
    std::string text;
    bool resize_to_text_extents = true;
    Anchor label_anchor = Anchor::CENTER;
    u32 vao = 0;
    u32 vbo = 0;
    std::vector<vertex2> vertices;
    glm::vec3 text_color = {1.0, 1.0, 1.0};
    vec2f text_extents{};

  public:
    Label(UI&);
    Label(UI&, std::string_view);
    Label(UI&, std::u32string_view);

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

    void set_text(std::u32string_view text_) {
      set_text(utf32_to_utf8(text_));
    }

    void set_text_color(glm::vec3 text_color_) {
      if (text_color == text_color_) { return; }
      text_color = text_color_;
      // text_dirty = true;
      dirty = true;
    }

    void append_text(std::string append) {
      if (append.empty()) { return; }
      text += append;
      // text_dirty = true;
      dirty = true;
    }

    void erase_last_character() {
      if (text.length() > 0) {
        text.pop_back();
        // text_dirty = true;
        dirty = true;
      }
    }

    void update() override;

    void draw() override;

  protected:
    void update_mesh();

    void setup_buffers();
};
