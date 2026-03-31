#include <memory>
#include <optional>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"
#include "font_face.hpp"
#include "freetype/freetype.h"
#include "label.hpp"
#include "opengl_includes.hpp"
#include "shader.hpp"
#include "sprite.hpp"
#include "ui.hpp"
#include "widget.hpp"

UI::UI(i32 window_width_, i32 window_height_)
  : sprite_shader{"sprite"}, text_shader{"text"} {
  window_width = window_width_;
  window_height = window_height_;
  if (FT_Init_FreeType(&freetype_lib)) {
    debug_error("failed to initialize freetype");
    return;
  }
  matrix = glm::mat4{1.0};
  matrix = glm::scale(matrix, glm::vec3(1.0f, -1.0f, 1.0f));

  font_face = FontFace(freetype_lib, "./assets/Arimo/Arimo-VariableFont_wght.ttf", 15);
}

void UI::process_input() {
  std::erase_if(widgets, [](auto&& w) { return w->get_marked_for_deletion(); });

  for (auto&& widget : widgets_to_add) {
    widgets.emplace_back(std::move(widget));
  }
  widgets_to_add.clear();

  for (auto&& widget : widgets) {
    if (!widget->get_is_updated()) { continue; }
    if (widget->get_is_drawn_on_top()) {
      widget->process_input();
    }
  }

  for (auto&& widget : widgets) {
    if (!widget->get_is_updated()) { continue; }
    if (!widget->get_is_drawn_on_top()) {
      widget->process_input();
    }
  }
}

void UI::update(i32 window_width_, i32 window_height_) {
  window_width = window_width_;
  window_height = window_height_;

  for (auto&& widget : widgets) {
    update_widget_recursive(widget);
  }
}

void UI::draw() {
  glDisable(GL_DEPTH_TEST);

  std::vector<Widget*> to_be_drawn_later;

  for (auto&& widget : widgets) {
    draw_widget_recursive(widget.get(), &to_be_drawn_later);
  }

  for (auto* widget : to_be_drawn_later) {
    draw_widget_recursive(widget, nullptr);
  }
}

void UI::update_widget_recursive(std::unique_ptr<Widget>& widget) {
  if (!widget->get_is_updated()) { return; }

  widget->set_window_width(window_width);
  widget->set_window_height(window_height);

  if (!widget->get_update_children_first()) { widget->update(); }

  for (auto&& child : widget->get_children()) {
    child->set_window_width(window_width);
    child->set_window_height(window_height);
    update_widget_recursive(child);
  }

  if (widget->get_update_children_first()) { widget->update(); }
}

void UI::draw_widget_recursive(Widget* widget, std::vector<Widget*>* to_be_drawn_later, std::optional<rect2i> parent_scissor_rect) {
  if (!widget->get_is_drawn()) { return; }

  if (to_be_drawn_later && widget->get_is_drawn_on_top()) {
    to_be_drawn_later->emplace_back(widget);
    return;
  }

  std::optional<rect2i> my_scissor_rect{};
  if (widget->get_clip_children()) {
    my_scissor_rect = {.begin = {widget->get_position(Anchor::TOP_LEFT).x,
                                 window_height - widget->get_position(Anchor::BOTTOM_RIGHT).y},
                       .size = {widget->get_width(),
                                widget->get_height()}};
    if (parent_scissor_rect.has_value()) {
      my_scissor_rect = my_scissor_rect->intersected(*parent_scissor_rect);
    }
  } else {
    my_scissor_rect = parent_scissor_rect;
  }

  for (auto&& child : widget->get_children()) {
    if (child->get_draw_behind_parent()) {
      draw_widget_recursive(child.get(), to_be_drawn_later, my_scissor_rect);
    }
  }

  if (widget->get_is_self_drawn()) {
    if (parent_scissor_rect.has_value()) {
      glEnable(GL_SCISSOR_TEST);
      glScissor(parent_scissor_rect->begin.x, parent_scissor_rect->begin.y, parent_scissor_rect->size.x, parent_scissor_rect->size.y);
    } else {
      glDisable(GL_SCISSOR_TEST);
    }
    widget->draw();
  }

  for (auto&& child : widget->get_children()) {
    if (!child->get_draw_behind_parent()) {
      draw_widget_recursive(child.get(), to_be_drawn_later, my_scissor_rect);
    }
  }
}
