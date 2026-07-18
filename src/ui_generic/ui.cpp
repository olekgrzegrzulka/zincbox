#include <memory>
#include <optional>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "common/input.hpp"
#include "common/types.hpp"
#include "freetype/freetype.h"
#include "label.hpp"
#include "opengl_includes.hpp"
#include "shader.hpp"
#include "sprite.hpp"
#include "ui.hpp"
#include "widget.hpp"

static constexpr char shader_frag[] = {
#embed "../shaders/ui.frag"
  , 0};
static constexpr char shader_vert[] = {
#embed "../shaders/ui.vert"
  , 0};

UI::UI(i32 window_width_, i32 window_height_) : shader{shader_vert, shader_frag} {
  window_width = window_width_;
  window_height = window_height_;
  if (FT_Init_FreeType(&freetype_lib)) {
    out::critical("failed to initialize freetype");
    exit(1);
  }
  matrix = glm::mat4{1.0};
  matrix = glm::scale(matrix, glm::vec3(1.0f, -1.0f, 1.0f));

  // font_face = FontFace(freetype_lib, "./assets/Arimo/Arimo-VariableFont_wght.ttf", 14);
  // font_face = FontFace(freetype_lib, "./assets/arial/ARIAL.TTF", 15);
}

static void update_hovered_widgets_recursive(Widget* widget, std::vector<Widget*>& hovered_widgets,
                                             bool parent_hovered = true) {
  vec2i mouse_pos = Input::get_mouse_pos();
  vec2i widget_top_left = widget->get_position(Anchor::TOP_LEFT);
  vec2i widget_bottom_right = widget->get_position(Anchor::BOTTOM_RIGHT);
  bool test_x = mouse_pos.x >= widget_top_left.x && mouse_pos.x < widget_bottom_right.x;
  bool test_y = mouse_pos.y >= widget_top_left.y && mouse_pos.y < widget_bottom_right.y;
  bool hovered = test_x && test_y;
  if (widget->get_hover_test_parent()) { hovered &= parent_hovered; }

  if (!widget->get_is_updated() || !widget->get_is_drawn()) { return; }

  if (widget->get_is_self_drawn() && hovered) { hovered_widgets.emplace_back(widget); }
  for (auto&& child : widget->get_children()) {
    update_hovered_widgets_recursive(child.get(), hovered_widgets, parent_hovered && hovered);
  }
}

void UI::input() {
  std::erase_if(widgets, [](auto&& w) { return w->get_marked_for_deletion(); });

  hovered_widgets.clear();
  for (auto&& widget : widgets) {
    update_hovered_widgets_recursive(widget.get(), hovered_widgets);
  }

  for (auto&& widget : widgets_to_add) {
    widgets.emplace_back(std::move(widget));
  }
  widgets_to_add.clear();

  for (auto&& widget = widgets.rbegin(); widget != widgets.rend(); widget += 1) {
    if (!widget->get()->get_is_updated()) { continue; }
    if (widget->get()->get_is_drawn_on_top()) { widget->get()->input(); }
  }

  for (auto&& widget = widgets.rbegin(); widget != widgets.rend(); widget += 1) {
    if (!widget->get()->get_is_updated()) { continue; }
    if (!widget->get()->get_is_drawn_on_top()) { widget->get()->input(); }
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

  shader.use();
  shader.set_uniform_mat4("matrix", get_matrix());

  // FIXME: don't hardcode texture size
  shader.set_uniform_float("tex_size", 16, 16);

  texture_atlas.bind(0);
  font_face.bind(1);

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

void UI::draw_widget_recursive(Widget* widget, std::vector<Widget*>* to_be_drawn_later,
                               std::optional<rect2i> parent_scissor_rect) {
  if (!widget->get_is_drawn()) { return; }

  if (to_be_drawn_later && widget->get_is_drawn_on_top()) {
    to_be_drawn_later->emplace_back(widget);
    return;
  }

  std::optional<rect2i> clipped_rect;
  if (widget->get_clip_children() || widget->get_clip()) {
    rect2i widget_rect = {
      .begin = {widget->get_position(Anchor::TOP_LEFT).x, window_height - widget->get_position(Anchor::BOTTOM_RIGHT).y},
      .size = {widget->get_width(), widget->get_height()}};
    clipped_rect = parent_scissor_rect ? parent_scissor_rect->intersected(widget_rect) : widget_rect;
  }

  std::optional<rect2i> my_scissor_rect = widget->get_clip_children() ? clipped_rect : parent_scissor_rect;
  for (auto&& child : widget->get_children()) {
    if (child->get_draw_behind_parent()) { draw_widget_recursive(child.get(), to_be_drawn_later, my_scissor_rect); }
  }
  if (widget->get_is_self_drawn()) {
    std::optional<rect2i> scissor_rect = widget->get_clip() ? clipped_rect : parent_scissor_rect;
    if (scissor_rect) {
      glEnable(GL_SCISSOR_TEST);
      glScissor(scissor_rect->begin.x, scissor_rect->begin.y, scissor_rect->size.x, scissor_rect->size.y);
    } else {
      glDisable(GL_SCISSOR_TEST);
    }
    widget->draw();
  }
  for (auto&& child : widget->get_children()) {
    if (!child->get_draw_behind_parent()) { draw_widget_recursive(child.get(), to_be_drawn_later, my_scissor_rect); }
  }
}
