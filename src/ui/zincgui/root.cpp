#include "root.hpp"
#include <iterator>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>
#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <stdlib.h>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "freetype/freetype.h"
#include "shader.hpp"
#include "shader_src.hpp"
#include "ui/zincgui/font_face.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/widget.hpp"

namespace {
  void ensure_buffers(u32& vao, u32& vbo);
}

zincgui::Root::Root(i32 window_width_, i32 window_height_) : m_shader{shader_vert, shader_frag} {
  m_window_width = window_width_;
  m_window_height = window_height_;
  if (FT_Init_FreeType(&m_freetype_lib)) {
    out::critical("failed to initialize freetype");
    std::exit(1);
  }
  m_matrix = glm::mat4{1.0};
  m_matrix = glm::scale(m_matrix, glm::vec3(1.0f, -1.0f, 1.0f));
}

zincgui::Root::~Root() {
  m_widgets.clear();
  m_widgets_to_add.clear();
  m_fonts.clear();
  if (m_freetype_lib != nullptr) {
    FT_Done_FreeType(m_freetype_lib);
    m_freetype_lib = nullptr;
  }
}

size_t zincgui::Root::add_font(std::string_view name, const std::string& path) {
  size_t id = m_fonts.size();
  m_fonts.emplace_back(std::make_unique<FontFace>(m_freetype_lib, path));
  m_font_name_to_id[std::string(name)] = id;
  return id;
}

size_t zincgui::Root::add_font_from_data(std::string_view name, const void* data, size_t data_size) {
  size_t id = m_fonts.size();
  m_fonts.emplace_back(std::make_unique<FontFace>(m_freetype_lib, data, data_size));
  m_font_name_to_id[std::string(name)] = id;
  return id;
}

std::optional<size_t> zincgui::Root::find_font_id(std::string_view name) const {
  auto it = m_font_name_to_id.find(std::string(name));
  if (it == m_font_name_to_id.end()) { return std::nullopt; }
  return it->second;
}

zincgui::FontFace& zincgui::Root::get_font(std::string_view name) {
  auto id_opt = find_font_id(name);
  if (!id_opt.has_value()) {
    out::critical("requested unknown font '{}'", name);
    std::exit(1);
  }
  return *m_fonts[*id_opt];
}

const zincgui::FontFace& zincgui::Root::get_font(std::string_view name) const {
  auto id_opt = find_font_id(name);
  if (!id_opt.has_value()) {
    out::critical("requested unknown font '{}'", name);
    std::exit(1);
  }
  return *m_fonts[*id_opt];
}

zincgui::FontFace& zincgui::Root::get_font(size_t id) {
  ensure(id < m_fonts.size());
  return *m_fonts[id];
}

const zincgui::FontFace& zincgui::Root::get_font(size_t id) const {
  ensure(id < m_fonts.size());
  return *m_fonts[id];
}

void zincgui::Root::construct_traversal_order(Widget* widget, i32 always_on_top_layer,
                                              std::vector<std::vector<Widget*>>& layers) {

  widget->dirty = false;

  if (widget->parent && widget->parent->deleted_) { widget->deleted_ = true; }

  if (widget->get_marked_for_deletion() && !widget->deleted_) {
    widget->deleted_ = true;
    m_deleted_widgets.insert(widget);
  }

  if (widget->deleted_) {
    if (m_focused_widget == widget) { m_focused_widget = nullptr; }
    if (m_focus_manager == widget) { m_focus_manager = nullptr; }
  }

  if (!widget->deleted_) {
    if (!widget->children_to_add.empty() && !widget->deleted_) { m_mutated_widgets.insert(widget); }

    if (widget->is_drawn_on_top) { always_on_top_layer += 1; }
    Widget* parent = widget->parent;

    bool parent_drawn = parent ? parent->visible_ : true;
    widget->visible_ = parent_drawn && widget->get_is_drawn();

    bool parent_updated = parent ? parent->updated_ : true;
    widget->updated_ = parent_updated && widget->get_is_updated();

    bool parent_dirty = parent ? parent->dirty : false;
    widget->dirty = parent_dirty || widget->dirty;

    vec2i origin{};
    if (widget->parent) {
      origin = widget->parent->get_position(widget->parent_anchor);
    } else {
      origin = vec2i(anchor_to_uv(widget->parent_anchor) * vec2f{get_window_size()});
    }

    widget->global_position_ =
      origin + vec2i(widget->x, widget->y) - vec2i(vec2f{widget->width, widget->height} * anchor_to_uv(widget->anchor));

    std::optional<rect2i> parent_scissor = parent ? parent->scissor_ : std::nullopt;
    if (widget->is_drawn_on_top) { parent_scissor = std::nullopt; }
    if (widget->get_clip_children()) {
      widget->scissor_ = widget->get_rect();
      if (parent && parent->scissor_.has_value()) {
        widget->scissor_ = widget->scissor_->intersected(*parent->scissor_);
      }
    } else {
      widget->scissor_ = parent_scissor;
    }

    vec2i mouse_pos = Input::get_mouse_pos();
    vec2i widget_top_left = widget->get_position(Anchor::TOP_LEFT);
    vec2i widget_bottom_right = widget->get_position(Anchor::BOTTOM_RIGHT);
    bool hovered = (mouse_pos.x >= widget_top_left.x && mouse_pos.x < widget_bottom_right.x) &&
                   (mouse_pos.y >= widget_top_left.y && mouse_pos.y < widget_bottom_right.y);
    bool parent_hovered = !widget->parent || widget->parent->hovered_;
    if (widget->updated_ && widget->get_is_self_drawn() && hovered && parent_hovered) {
      // m_hovered_widgets.emplace_back(widget);
      // widget->hovered_ = true;
    } else {
      // widget->hovered_ = false;
    }
  }

  for (auto&& child : widget->get_children()) {
    if (child->get_draw_behind_parent() && !child->get_is_drawn_on_top()) {
      construct_traversal_order(child.get(), always_on_top_layer, layers);
    }
  }

  if ((i32)layers.size() <= always_on_top_layer) { layers.resize(always_on_top_layer + 1); }
  layers[always_on_top_layer].emplace_back(widget);

  if (!widget->deleted_ && widget->visible_ && widget->is_self_drawn) {
    rect2i scissor_rect = widget->scissor_.value_or(rect2i{{-1, -1}, {0, 0}});
    if (widget->scissor_ && widget->get_clip()) {
      rect2i widget_rect = widget->get_rect();
      scissor_rect = widget->scissor_->intersected(widget_rect);
    }
    std::vector<Vertex> verts = widget->get_vertices();
    for (auto&& v : verts) {
      v.scissor_begin = scissor_rect.begin;
      v.scissor_end = scissor_rect.begin + scissor_rect.size;
    }

    if ((i32)m_vertex_layers.size() <= always_on_top_layer) { m_vertex_layers.resize(always_on_top_layer + 1); }
    auto& dst = m_vertex_layers[always_on_top_layer];
    if (!verts.empty()) {
      dst.insert(dst.end(), verts.begin(), verts.end());
      m_vertices_count += verts.size();
    }
  }

  for (auto&& child : widget->get_children()) {
    if (!child->get_draw_behind_parent() && !child->get_is_drawn_on_top()) {
      construct_traversal_order(child.get(), always_on_top_layer, layers);
    }
  }

  for (auto&& child : widget->get_children()) {
    if (child->get_is_drawn_on_top()) { construct_traversal_order(child.get(), always_on_top_layer, layers); }
  }
}

void zincgui::Root::construct_traversal_order() {
  m_tree_dirty = false;
  m_widget_traversal_order.clear();
  m_deleted_widgets.clear();
  m_mutated_widgets.clear();
  m_hovered_widgets.clear();
  m_vertex_layers.clear();
  m_vertices_count = 0;

  std::vector<std::vector<Widget*>> layers = {std::vector<Widget*>{}};

  for (auto&& w : m_widgets) {
    if (w->get_draw_behind_parent() && !w->get_is_drawn_on_top()) { construct_traversal_order(w.get(), 0, layers); }
  }
  for (auto&& w : m_widgets) {
    if (!w->get_draw_behind_parent() && !w->get_is_drawn_on_top()) { construct_traversal_order(w.get(), 0, layers); }
  }
  for (auto&& w : m_widgets) {
    if (w->get_is_drawn_on_top()) { construct_traversal_order(w.get(), 0, layers); }
  }

  for (auto& layer : layers) {
    m_widget_traversal_order.reserve(m_widget_traversal_order.size() + layer.size());
    for (auto* widget : layer) {
      m_widget_traversal_order.emplace_back(widget);
    }
  }

  ensure_buffers(m_vao, m_vbo);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices_count * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);
  GLintptr offset = 0;
  for (auto& vertices : m_vertex_layers) {
    if (vertices.empty()) { continue; }
    GLsizeiptr size_bytes = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
    glBufferSubData(GL_ARRAY_BUFFER, offset, size_bytes, vertices.data());
    offset += size_bytes;
  }
}

bool zincgui::Root::process_mutations() {
  bool changed = false;

  if (!m_deleted_widgets.empty()) {
    changed = true;
    for (Widget* w : m_deleted_widgets) {
      if (w->parent != nullptr) {
        std::erase_if(w->parent->children, [w](const auto& child) { return child.get() == w; });
      } else {
        std::erase_if(m_widgets, [w](const auto& child) { return child.get() == w; });
      }
    }
    m_deleted_widgets.clear();
  }

  if (!m_widgets_to_add.empty()) {
    changed = true;
    m_widgets.insert(m_widgets.end(), std::move_iterator(m_widgets_to_add.begin()),
                     std::move_iterator(m_widgets_to_add.end()));
    m_widgets_to_add.clear();
  }

  if (!m_mutated_widgets.empty()) {
    changed = true;
    for (Widget* w : m_mutated_widgets) {
      w->last_update_frame_ = 0;
      w->children.insert(w->children.end(), std::move_iterator(w->children_to_add.begin()),
                         std::move_iterator(w->children_to_add.end()));
      w->children_to_add.clear();
    }
  }

  return changed;
}

void zincgui::Root::tick(i32 window_width_, i32 window_height_) {
  m_current_frame += 1;

  if (window_width_ != m_window_width || window_height_ != m_window_height) {
    m_tree_dirty = true;
    for (auto&& widget : m_widgets) {
      widget->mark_dirty();
    }
  }

  m_window_width = window_width_;
  m_window_height = window_height_;

  if (!m_widgets_to_add.empty()) {
    m_tree_dirty = true;
    m_widgets.insert(m_widgets.end(), std::move_iterator(m_widgets_to_add.begin()),
                     std::move_iterator(m_widgets_to_add.end()));
    m_widgets_to_add.clear();
  }

  if (m_tree_dirty) { construct_traversal_order(); }

  while (process_mutations()) {
    construct_traversal_order();
  }

  bool hover_claimed = false;
  for (auto it = m_widget_traversal_order.rbegin(); it != m_widget_traversal_order.rend(); it += 1) {
    Widget* w = *it;

    if (!w->updated_ || !w->visible_ || w->deleted_) {
      w->hovered_ = false;
      continue;
    }

    auto mouse_pos = Input::get_mouse_pos();

    bool inside = false;
    if (w->scissor_) {
      auto s = w->scissor_->intersected(w->get_rect());
      s.begin.y = window_height_ - s.begin.y - s.size.y; // FIXME : get_rect() returns inverted y axis...
      inside = mouse_pos.x >= s.begin.x && mouse_pos.x < s.begin.x + s.size.x && mouse_pos.y >= s.begin.y &&
               mouse_pos.y < s.begin.y + s.size.y;
    } else {
      inside =
        mouse_pos.x >= w->get_position(Anchor::TOP_LEFT).x && mouse_pos.x < w->get_position(Anchor::BOTTOM_RIGHT).x &&
        mouse_pos.y >= w->get_position(Anchor::TOP_LEFT).y && mouse_pos.y < w->get_position(Anchor::BOTTOM_RIGHT).y;
    }

    if (!hover_claimed && inside) {
      switch (w->hit_test_behavior) {
      case HitTestBehavior::IGNORE: {
        w->hovered_ = false;
      }
      case HitTestBehavior::BLOCK: {
        w->hovered_ = true;
        hover_claimed = true;
        break;
      }
      case HitTestBehavior::PASS_THROUGH: {
        w->hovered_ = true;
        break;
      }
      }
    } else {
      w->hovered_ = false;
    }
  }

  for (auto& ev : Input::get_event_queue()) {

    if (m_focused_widget && m_focused_widget->updated_) {
      if (auto* ev_key = std::get_if<Input::InputEventKey>(&ev)) {
        if (ev_key->action == Input::KeyAction::PRESS || ev_key->action == Input::KeyAction::REPEAT) {
          if (ev_key->key == Input::Key::KEY_ENTER || ev_key->key == Input::Key::KEY_ENTER2 ||
              ev_key->key == Input::Key::KEY_KP_ENTER) {
            m_focused_widget->focus_accept();
          }
        }
      }

      std::visit(
        [this](auto& ev_) {
          if (!ev_.handled) { m_focused_widget->event(ev_); }
        },
        ev);
    }

    if (m_focus_manager && m_focus_manager->updated_) {
      if (auto* ev_key = std::get_if<Input::InputEventKey>(&ev)) {
        if (ev_key->action == Input::KeyAction::PRESS || ev_key->action == Input::KeyAction::REPEAT) {
          if (ev_key->key == Input::Key::KEY_LEFT) {
            set_focused_widget(m_focus_manager->focus_manager_left());
            ev_key->handled = true;
          } else if (ev_key->key == Input::Key::KEY_RIGHT) {
            set_focused_widget(m_focus_manager->focus_manager_right());
            ev_key->handled = true;
          } else if (ev_key->key == Input::Key::KEY_UP) {
            set_focused_widget(m_focus_manager->focus_manager_up());
            ev_key->handled = true;
          } else if (ev_key->key == Input::Key::KEY_DOWN) {
            set_focused_widget(m_focus_manager->focus_manager_down());
            ev_key->handled = true;
          } else if (ev_key->key == Input::Key::KEY_TAB) {
            set_focused_widget(m_focus_manager->focus_manager_next());
            ev_key->handled = true;
          }
        }
      }
    }

    bool replace_focused_widget = false;
    Widget* new_focused_widget = nullptr;
    Widget* new_focus_manager = nullptr;

    for (auto it = m_widget_traversal_order.rbegin(); it != m_widget_traversal_order.rend(); it += 1) {
      Widget* w = *it;
      w->set_window_width(m_window_width);
      w->set_window_height(m_window_height);

      using MA = Input::MouseAction;
      using MB = Input::MouseButton;

      if (w->updated_ && w != m_focused_widget) {
        std::visit(
          [it, w](auto& ev_) {
            if (!ev_.handled) { w->event(ev_); }
          },
          ev);
      }

      if (auto* ev_ = std::get_if<Input::InputEventMouseButton>(&ev)) {
        if (ev_->action == MA::PRESS &&
            (ev_->button == MB::MOUSE_BUTTON_LEFT || ev_->button == MB::MOUSE_BUTTON_RIGHT)) {
          replace_focused_widget = true;
          if (w->get_is_focusable() && w->is_mouse_hovering()) {
            new_focused_widget = w;
          } else if (w->get_can_be_focus_manager() && w->is_mouse_hovering()) {
            new_focus_manager = w;
          }
        }
      }
    }

    if (replace_focused_widget && m_focus_manager != new_focus_manager) {
      m_focus_manager = new_focus_manager;
      if (m_focus_manager) { set_focused_widget(m_focus_manager->focus_manager_next()); }
    } else if (replace_focused_widget) {
      set_focused_widget(new_focused_widget);
    }
  }

  bool tree_changed = false;
  do {
    for (auto* widget : m_widget_traversal_order) {
      if (widget->updated_ && widget->last_update_frame_ != m_current_frame) {
        widget->last_update_frame_ = m_current_frame;
        widget->input();
        widget->update();
      }
    }

    if (m_tree_dirty) { construct_traversal_order(); }
    tree_changed = process_mutations();
    if (tree_changed) { construct_traversal_order(); }
  } while (tree_changed || !m_deleted_widgets.empty() || !m_mutated_widgets.empty() || !m_widgets_to_add.empty());
}

void zincgui::Root::set_focused_widget(Widget* w) {
  if (w && (w->get_marked_for_deletion() || w->deleted_)) { return; }
  if (m_focused_widget == w) { return; }
  if (m_focused_widget) { m_focused_widget->on_unfocused(); }
  m_focused_widget = w;
  if (m_focused_widget) { m_focused_widget->on_focused(); }
}

void zincgui::Root::set_focus_manager(Widget* w) {
  if (w && (w->get_marked_for_deletion() || w->deleted_)) { return; }
  m_focus_manager = w;
}

void zincgui::Root::draw() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  m_shader.use();
  m_shader.set_uniform_mat4("matrix", get_matrix());
  m_shader.set_uniform_float("tex_size", m_texture_atlas.size(), m_texture_atlas.size());
  for (size_t i = 0; i < m_fonts.size(); i += 1) {
    m_fonts[i]->bind(0);
  }
  m_texture_atlas.bind(1);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ensure_buffers(m_vao, m_vbo);
  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  if (!m_vertex_layers.empty()) { glDrawArrays(GL_TRIANGLES, 0, m_vertices_count); }
}

namespace {
  void ensure_buffers(u32& vao, u32& vbo) {
    using zincgui::Vertex;
    if (vao == 0) {
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);

      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      glEnableVertexAttribArray(0);
      glVertexAttribIPointer(0, 1, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, type));

      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nine_slice_margin));

      glEnableVertexAttribArray(4);
      glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nine_slice_scale));

      glEnableVertexAttribArray(5);
      glVertexAttribIPointer(5, 2, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, widget_size));

      glEnableVertexAttribArray(6);
      glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv_start));

      glEnableVertexAttribArray(7);
      glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv_end));

      glEnableVertexAttribArray(8);
      glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, atlas_i));

      glEnableVertexAttribArray(9);
      glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

      glEnableVertexAttribArray(10);
      glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, opacity));

      glEnableVertexAttribArray(11);
      glVertexAttribPointer(11, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, scissor_begin));

      glEnableVertexAttribArray(12);
      glVertexAttribPointer(12, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, scissor_end));
    }
  }
} // namespace
