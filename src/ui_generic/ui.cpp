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

void UI::update_hovered_widgets_recursive(Widget* widget) {
  vec2i mouse_pos = Input::get_mouse_pos();
  vec2i widget_top_left = widget->get_position(Anchor::TOP_LEFT);
  vec2i widget_bottom_right = widget->get_position(Anchor::BOTTOM_RIGHT);
  bool hovered = (mouse_pos.x >= widget_top_left.x && mouse_pos.x < widget_bottom_right.x) &&
                 (mouse_pos.y >= widget_top_left.y && mouse_pos.y < widget_bottom_right.y);
  bool parent_hovered = !widget->parent || widget->parent->hovered_;
  if (!widget->get_marked_for_deletion() && widget->get_is_updated() && widget->get_is_drawn() &&
      widget->get_is_self_drawn() && hovered && parent_hovered) {
    hovered_widgets.emplace_back(widget);
    widget->hovered_ = true;
  } else {
    widget->hovered_ = false;
  }

  for (auto&& child : widget->get_children()) {
    update_hovered_widgets_recursive(child.get());
  }
}

void UI::construct_traversal_order(Widget* widget) {
  Widget* parent = widget->parent;

  bool parent_drawn = parent ? parent->visible_ : true;
  widget->visible_ = parent_drawn && widget->get_is_drawn();

  bool parent_updated = parent ? parent->updated_ : true;
  widget->updated_ = parent_updated && widget->get_is_updated();

  std::optional<rect2i> parent_scissor = parent ? parent->scissor_ : std::nullopt;
  if (widget->is_drawn_on_top) { parent_scissor = std::nullopt; }
  if (widget->get_clip_children()) {
    widget->scissor_ = {
      .begin = {widget->get_position(Anchor::TOP_LEFT).x, window_height - widget->get_position(Anchor::BOTTOM_RIGHT).y},
      .size = {widget->get_width(), widget->get_height()}};
    if (parent && parent->scissor_.has_value()) { widget->scissor_ = widget->scissor_->intersected(*parent->scissor_); }
  } else {
    widget->scissor_ = parent_scissor;
  }

  for (auto&& child : widget->get_children()) {
    if (child->get_draw_behind_parent() && !child->get_is_drawn_on_top()) { construct_traversal_order(child.get()); }
  }

  widget_traversal_order.emplace_back(widget);

  for (auto&& child : widget->get_children()) {
    if (!child->get_draw_behind_parent() && !child->get_is_drawn_on_top()) { construct_traversal_order(child.get()); }
  }

  for (auto&& child : widget->get_children()) {
    if (child->get_is_drawn_on_top()) { construct_traversal_order(child.get()); }
  }
}

void UI::construct_traversal_order() {
  widget_traversal_order.clear();

  for (auto&& w : widgets) {
    if (w->get_draw_behind_parent()) { construct_traversal_order(w.get()); }
  }
  for (auto&& w : widgets) {
    if (!w->get_draw_behind_parent() && !w->get_is_drawn_on_top()) { construct_traversal_order(w.get()); }
  }
  for (const auto& w : widgets) {
    if (w->get_is_drawn_on_top()) { construct_traversal_order(w.get()); }
  }
}

bool UI::rebuild_tree(Widget* widget, bool tree_changed) {
  std::erase_if(widget->children, [&tree_changed](auto&& w) {
    tree_changed |= w->get_marked_for_deletion();
    return w->get_marked_for_deletion();
  });

  if (!widget->children_to_add.empty()) {
    tree_changed = true;
    widget->children.insert(widget->children.end(), std::move_iterator(widget->children_to_add.begin()),
                            std::move_iterator(widget->children_to_add.end()));
    widget->children_to_add.clear();
  }

  for (auto&& child : widget->get_children()) {
    if (rebuild_tree(child.get(), tree_changed)) { tree_changed = true; }
  }

  return tree_changed;
}

bool UI::rebuild_tree() {
  bool tree_changed = false;

  std::erase_if(widgets, [&tree_changed](auto&& w) {
    tree_changed |= w->get_marked_for_deletion();
    return w->get_marked_for_deletion();
  });

  if (!widgets_to_add.empty()) {
    tree_changed = true;
    widgets.insert(widgets.end(), std::move_iterator(widgets_to_add.begin()), std::move_iterator(widgets_to_add.end()));
    widgets_to_add.clear();
  }

  for (auto&& widget : widgets) {
    tree_changed |= rebuild_tree(widget.get());
  }

  return tree_changed;
}

void UI::input() {
  for (auto it = widget_traversal_order.rbegin(); it != widget_traversal_order.rend(); ++it) {
    if ((*it)->updated_) {
      for (auto& ev : Input::get_event_queue()) {
        std::visit([it](auto& ev) { (*it)->event(ev); }, ev);
      }
      (*it)->input();
    }
  }
}

void UI::rebuild() {
  bool tree_changed = rebuild_tree();
  if (tree_changed || true) { construct_traversal_order(); }

  hovered_widgets.clear();
  for (auto&& w : widgets) {
    update_hovered_widgets_recursive(w.get());
  }
}

void UI::update(i32 window_width_, i32 window_height_) {
  window_width = window_width_;
  window_height = window_height_;

  for (auto* widget : widget_traversal_order) {
    widget->set_window_width(window_width);
    widget->set_window_height(window_height);
    if (widget->updated_) { widget->update(); }
  }
}

void UI::draw() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  shader.use();
  shader.set_uniform_mat4("matrix", get_matrix());
  // FIXME: don't hardcode texture size
  shader.set_uniform_float("tex_size", 16, 16);
  texture_atlas.bind(0);
  font_face.bind(1);
  for (auto* widget : widget_traversal_order) {
    if (widget->visible_ && widget->is_self_drawn) {
      if (widget->scissor_.has_value()) {
        rect2i scissor_rect = widget->scissor_.value();
        if (widget->get_clip()) {
          rect2i widget_rect = {.begin = {widget->get_position(Anchor::TOP_LEFT).x,
                                          window_height - widget->get_position(Anchor::BOTTOM_RIGHT).y},
                                .size = {widget->get_width(), widget->get_height()}};
          scissor_rect = scissor_rect.intersected(widget_rect);
        }
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissor_rect.begin.x, scissor_rect.begin.y, scissor_rect.size.x, scissor_rect.size.y);
      } else {
        glDisable(GL_SCISSOR_TEST);
      }
      widget->draw();
    }
  }
}
