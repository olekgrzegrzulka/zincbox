#include "ui/zincgui/label.hpp"
#include <algorithm>
#include <cmath>
#include <span>
#include <string_view>
#include <glad/glad.h>
#include <stddef.h>
#include "common/types.hpp"
#include "ui/theme.hpp" // FIXME zincgui should not depend on ui/theme.hpp
#include "ui/theme_config.hpp"
#include "ui/zincgui/font_face.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/vertex.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

namespace {

  struct TextStyle {
      bool bold = false;
      bool italic = false;
      bool underline = false;
      bool strikethrough = false;
      vec3f color{1.0f, 1.0f, 1.0f};

      bool operator==(const TextStyle&) const = default;
  };

  struct StyleRun {
      u32 byte_offset = 0;
      u32 byte_length = 0;
      TextStyle style{};
  };

  struct ShapedGlyph {
      u32 glyph_index = 0;
      const zincgui::FontGlyph* cached_glyph = nullptr;
      float x_advance = 0.0f;
      float y_advance = 0.0f;
      float x_offset = 0.0f;
      float y_offset = 0.0f;
      bool is_space = false;
      TextStyle style{};
  };

  struct ShapedLine {
      std::vector<ShapedGlyph> glyphs;
      float width = 0.0f;
      i32 space_count = 0;
      bool is_last_in_paragraph = true;
  };

  bool parse_hex_color(std::string_view hex, vec3f& out_color) {
    if (hex.starts_with('#')) { hex.remove_prefix(1); }
    if (hex.size() != 6) { return false; }
    u32 val = 0;
    for (char c : hex) {
      val <<= 4;
      if (c >= '0' && c <= '9') {
        val |= static_cast<u32>(c - '0');
      } else if (c >= 'a' && c <= 'f') {
        val |= static_cast<u32>(c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        val |= static_cast<u32>(c - 'A' + 10);
      } else {
        return false;
      }
    }
    out_color.r = static_cast<float>((val >> 16) & 0xFF) / 255.0f;
    out_color.g = static_cast<float>((val >> 8) & 0xFF) / 255.0f;
    out_color.b = static_cast<float>(val & 0xFF) / 255.0f;
    return true;
  }

  std::string_view parse_paragraph_runs(std::string_view raw_paragraph, bool parse_tags, const vec3f& base_color,
                                        i32& bold_depth, i32& italic_depth, i32& underline_depth, i32& strike_depth,
                                        std::vector<vec3f>& color_stack, std::string& clean_text,
                                        std::vector<StyleRun>& runs) {
    runs.clear();

    auto current_style = [&]() -> TextStyle {
      return TextStyle{
        .bold = bold_depth > 0,
        .italic = italic_depth > 0,
        .underline = underline_depth > 0,
        .strikethrough = strike_depth > 0,
        .color = color_stack.empty() ? base_color : color_stack.back(),
      };
    };

    if (!parse_tags || raw_paragraph.find('<') == std::string_view::npos) {
      if (!raw_paragraph.empty()) {
        runs.push_back(StyleRun{
          .byte_offset = 0,
          .byte_length = static_cast<u32>(raw_paragraph.size()),
          .style = current_style(),
        });
      }
      return raw_paragraph;
    }

    clean_text.clear();
    clean_text.reserve(raw_paragraph.size());

    TextStyle active_style = current_style();
    u32 run_start = 0;

    auto flush_run = [&]() {
      u32 cur_len = static_cast<u32>(clean_text.size());
      if (cur_len > run_start) {
        runs.push_back(StyleRun{
          .byte_offset = run_start,
          .byte_length = cur_len - run_start,
          .style = active_style,
        });
        run_start = cur_len;
      }
      active_style = current_style();
    };

    size_t i = 0;
    while (i < raw_paragraph.size()) {
      if (raw_paragraph[i] == '<') {
        size_t close_pos = raw_paragraph.find('>', i + 1);
        if (close_pos != std::string_view::npos && close_pos - i <= 16) {
          std::string_view tag = raw_paragraph.substr(i + 1, close_pos - i - 1);
          bool matched = true;

          if (tag == "b" || tag == "B") {
            flush_run();
            bold_depth += 1;
            active_style = current_style();
          } else if (tag == "/b" || tag == "/B") {
            flush_run();
            bold_depth = std::max(0, bold_depth - 1);
            active_style = current_style();
          } else if (tag == "i" || tag == "I") {
            flush_run();
            italic_depth += 1;
            active_style = current_style();
          } else if (tag == "/i" || tag == "/I") {
            flush_run();
            italic_depth = std::max(0, italic_depth - 1);
            active_style = current_style();
          } else if (tag == "u" || tag == "U") {
            flush_run();
            underline_depth += 1;
            active_style = current_style();
          } else if (tag == "/u" || tag == "/U") {
            flush_run();
            underline_depth = std::max(0, underline_depth - 1);
            active_style = current_style();
          } else if (tag == "s" || tag == "S") {
            flush_run();
            strike_depth += 1;
            active_style = current_style();
          } else if (tag == "/s" || tag == "/S") {
            flush_run();
            strike_depth = std::max(0, strike_depth - 1);
            active_style = current_style();
          } else if (tag == "/c" || tag == "/C" || tag == "/color" || tag == "/COLOR") {
            flush_run();
            if (!color_stack.empty()) { color_stack.pop_back(); }
            active_style = current_style();
          } else if (tag.starts_with("c=") || tag.starts_with("C=")) {
            vec3f parsed_col{};
            if (parse_hex_color(tag.substr(2), parsed_col)) {
              flush_run();
              color_stack.push_back(parsed_col);
              active_style = current_style();
            } else {
              matched = false;
            }
          } else if (tag.starts_with("color=") || tag.starts_with("COLOR=")) {
            vec3f parsed_col{};
            if (parse_hex_color(tag.substr(6), parsed_col)) {
              flush_run();
              color_stack.push_back(parsed_col);
              active_style = current_style();
            } else {
              matched = false;
            }
          } else {
            matched = false;
          }

          if (matched) {
            i = close_pos + 1;
            continue;
          }
        }
      }

      clean_text.push_back(raw_paragraph[i]);
      i += 1;
    }

    flush_run();
    return clean_text;
  }

} // namespace

zincgui::Label::Label(Root& ui_) : Widget::Widget(ui_) {
  set_text_color(theme::config().text_color);
  clip = true;
}

zincgui::Label::Label(Root& ui_, std::string_view text_) : Widget::Widget(ui_) {
  set_text_color(theme::config().text_color);
  set_text(text_);
  update_text();
  clip = true;
}

zincgui::Label::~Label() {}

void zincgui::Label::set_font(size_t index) {
  if (font_index == index) { return; }
  font_index = index;
  shape_dirty = true;
  last_atlas_version = -1;
  mark_dirty();
}

void zincgui::Label::set_font(std::string_view name) {
  auto id_opt = ui.find_font_id(name);
  if (id_opt) { set_font(*id_opt); }
}

i32 zincgui::Label::get_font_size() const {
  if (font_size > 0) { return font_size; }
  if (zincbox::font_size() > 0) { return zincbox::font_size(); }
  return ui.get_default_font_size();
}

void zincgui::Label::set_resize_to_text_extents(bool to) {
  if (to != resize_to_text_extents) {
    resize_to_text_extents = to;
    shape_dirty = true;
    mark_dirty();
  }
  if (resize_to_text_extents && (!parent || !parent->has_layout())) {
    if (shape_dirty) { update_text(); }
    set_width(static_cast<i32>(text_extents.x));
    set_height(static_cast<i32>(text_extents.y));
    set_min_width(static_cast<i32>(text_extents.x));
    set_min_height(static_cast<i32>(text_extents.y));
  }
}

void zincgui::Label::update() { Widget::update(); }

void zincgui::Label::update_text() {
  shape_dirty = false;
  vertices_dirty = true;
  local_quads.clear();
  text_extents = {};

  if (ui.get_font_count() == 0 || text.empty()) {
    last_layout_size = vec2i{width, height};
    return;
  }

  size_t active_font_idx = font_index < ui.get_font_count() ? font_index : 0;
  const FontFace& font = ui.get_font(active_font_idx);
  i32 px_size = get_font_size();
  const FontSizeMetrics& metrics = font.get_metrics(px_size);
  hb_buffer_t* hb_buf = font.acquire_hb_buffer();

  std::vector<ShapedLine> lines;
  i32 bold_depth = 0;
  i32 italic_depth = 0;
  i32 underline_depth = 0;
  i32 strike_depth = 0;
  std::vector<vec3f> color_stack;
  color_stack.push_back(text_color);

  std::string clean_paragraph_scratch;
  std::vector<StyleRun> paragraph_runs;

  size_t line_start = 0;
  while (line_start <= text.size()) {
    size_t next_nl = text.find('\n', line_start);
    std::string_view raw_para = (next_nl == std::string::npos)
                                  ? std::string_view(text).substr(line_start)
                                  : std::string_view(text).substr(line_start, next_nl - line_start);

    std::string_view para_view =
      parse_paragraph_runs(raw_para, parse_tags, text_color, bold_depth, italic_depth, underline_depth, strike_depth,
                           color_stack, clean_paragraph_scratch, paragraph_runs);

    std::vector<ShapedGlyph> para_glyphs;
    for (const auto& run : paragraph_runs) {
      hb_buffer_clear_contents(hb_buf);
      hb_buffer_add_utf8(hb_buf, para_view.data(), static_cast<int>(para_view.size()), run.byte_offset,
                         static_cast<int>(run.byte_length));
      hb_buffer_guess_segment_properties(hb_buf);
      hb_shape(metrics.hb_font, hb_buf, nullptr, 0);

      unsigned int glyph_count = 0;
      hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
      hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

      para_glyphs.reserve(para_glyphs.size() + glyph_count);
      for (unsigned int g = 0; g < glyph_count; ++g) {
        u32 cluster = glyph_info[g].cluster;
        bool is_ws = (cluster < para_view.size()) && (para_view[cluster] == ' ' || para_view[cluster] == '\t');
        float extra_bold_adv = run.style.bold ? 0.5f : 0.0f;

        para_glyphs.push_back(ShapedGlyph{
          .glyph_index = glyph_info[g].codepoint,
          .x_advance = (static_cast<float>(glyph_pos[g].x_advance) / 64.0f) + extra_bold_adv,
          .y_advance = static_cast<float>(glyph_pos[g].y_advance) / 64.0f,
          .x_offset = static_cast<float>(glyph_pos[g].x_offset) / 64.0f,
          .y_offset = static_cast<float>(glyph_pos[g].y_offset) / 64.0f,
          .is_space = is_ws,
          .style = run.style,
        });
      }
    }

    if (word_wrap && width > 0 && !para_glyphs.empty()) {
      float max_w = static_cast<float>(width);
      size_t idx = 0;
      while (idx < para_glyphs.size()) {
        ShapedLine current_line{};
        float accum_w = 0.0f;
        size_t last_space_glyph_idx = 0;
        float width_at_last_space = 0.0f;
        i32 spaces_at_last_space = 0;
        bool found_space = false;

        size_t scan = idx;
        while (scan < para_glyphs.size()) {
          const auto& sg = para_glyphs[scan];
          if (accum_w + sg.x_advance > max_w && scan > idx) { break; }
          if (sg.is_space) {
            last_space_glyph_idx = scan;
            width_at_last_space = accum_w;
            spaces_at_last_space = current_line.space_count;
            found_space = true;
            current_line.space_count += 1;
          }
          accum_w += sg.x_advance;
          current_line.glyphs.push_back(sg);
          scan += 1;
        }

        if (scan < para_glyphs.size() && found_space) {
          size_t keep_count = last_space_glyph_idx - idx;
          current_line.glyphs.resize(keep_count);
          current_line.width = width_at_last_space;
          current_line.space_count = spaces_at_last_space;
          current_line.is_last_in_paragraph = false;
          idx = last_space_glyph_idx + 1;
          while (idx < para_glyphs.size() && para_glyphs[idx].is_space) {
            idx += 1;
          }
        } else {
          current_line.width = accum_w;
          current_line.is_last_in_paragraph = (scan >= para_glyphs.size());
          idx = scan;
        }

        lines.push_back(std::move(current_line));
      }
    } else {
      ShapedLine single_line{};
      for (const auto& sg : para_glyphs) {
        single_line.width += sg.x_advance;
        if (sg.is_space) { single_line.space_count += 1; }
      }
      single_line.glyphs = std::move(para_glyphs);
      single_line.is_last_in_paragraph = true;
      lines.push_back(std::move(single_line));
    }

    if (next_nl == std::string::npos) { break; }
    line_start = next_nl + 1;
  }

  if (ellipsis && !word_wrap && width > 0) {
    u32 dot_glyph = font.get_codepoint_glyph_index('.');
    const FontGlyph* dot_fg = font.get_glyph(dot_glyph, px_size, false, false);
    float dot_adv = dot_fg ? static_cast<float>(dot_fg->size.x + 1) : 4.0f;
    float ellipsis_w = dot_adv * 3.0f;
    float max_w = static_cast<float>(width);

    for (auto& line : lines) {
      if (line.width > max_w && max_w > ellipsis_w) {
        float accum = 0.0f;
        size_t keep = 0;
        TextStyle last_style{.color = text_color};
        while (keep < line.glyphs.size() && accum + line.glyphs[keep].x_advance + ellipsis_w <= max_w) {
          accum += line.glyphs[keep].x_advance;
          last_style = line.glyphs[keep].style;
          keep += 1;
        }
        line.glyphs.resize(keep);
        for (int d = 0; d < 3; ++d) {
          line.glyphs.push_back(ShapedGlyph{
            .glyph_index = dot_glyph,
            .x_advance = dot_adv,
            .style = last_style,
          });
          accum += dot_adv;
        }
        line.width = accum;
      }
    }
  }

  for (const auto& line : lines) {
    text_extents.x = std::max(text_extents.x, std::ceil(line.width));
  }
  i32 lines_count = std::max<i32>(1, static_cast<i32>(lines.size()));
  text_extents.y = std::ceil((metrics.line_height * static_cast<float>(lines_count)) +
                             (line_spacing * static_cast<float>(lines_count - 1)));

  if (resize_to_text_extents && (!parent || !parent->has_layout())) {
    set_width(static_cast<i32>(text_extents.x));
    set_height(static_cast<i32>(text_extents.y));
    set_min_width(static_cast<i32>(text_extents.x));
    set_min_height(static_cast<i32>(text_extents.y));
  }

  vec2f anchor_uv = anchor_to_uv(label_anchor);
  TextAlign effective_align = text_alignment;
  if (!custom_alignment_set) {
    if (anchor_uv.x < 0.25f) {
      effective_align = TextAlign::LEFT;
    } else if (anchor_uv.x > 0.75f) {
      effective_align = TextAlign::RIGHT;
    } else {
      effective_align = TextAlign::CENTER;
    }
  }

  float block_offset_y = std::round(anchor_uv.y * (static_cast<float>(height) - text_extents.y));
  float target_box_width = static_cast<float>(width) > 0.0f ? static_cast<float>(width) : text_extents.x;

  u64 ver_before = font.get_atlas_version();
  size_t total_glyph_estimate = 0;
  for (auto& line : lines) {
    total_glyph_estimate += line.glyphs.size();
    for (auto& sg : line.glyphs) {
      sg.cached_glyph = font.get_glyph(sg.glyph_index, px_size, sg.style.bold, sg.style.italic);
    }
  }

  last_atlas_version = font.get_atlas_version();
  bool atlas_regrew = (last_atlas_version != ver_before);
  last_layout_size = vec2i{width, height};
  vec2f white_uv = font.get_white_uv();
  local_quads.reserve(total_glyph_estimate);

  float pen_y = block_offset_y + metrics.ascender;

  for (const auto& line : lines) {
    float line_start_x = 0.0f;
    float extra_space_advance = 0.0f;

    switch (effective_align) {
    case TextAlign::LEFT: {
      line_start_x = 0.0f;
      break;
    }
    case TextAlign::CENTER: {
      line_start_x = std::round((target_box_width - line.width) * 0.5f);
      break;
    }
    case TextAlign::RIGHT: {
      line_start_x = std::round(target_box_width - line.width);
      break;
    }
    case TextAlign::JUSTIFY: {
      line_start_x = 0.0f;
      if (!line.is_last_in_paragraph && line.space_count > 0 && target_box_width > line.width) {
        extra_space_advance = (target_box_width - line.width) / static_cast<float>(line.space_count);
      }
      break;
    }
    }

    float pen_x = line_start_x;

    for (const auto& sg : line.glyphs) {
      const FontGlyph* fg =
        atlas_regrew ? font.get_glyph(sg.glyph_index, px_size, sg.style.bold, sg.style.italic) : sg.cached_glyph;
      float adv_x = sg.x_advance + (sg.is_space ? extra_space_advance : 0.0f);

      if (fg != nullptr && fg->size.x > 0 && fg->size.y > 0) {
        float gx = std::round(pen_x + sg.x_offset + static_cast<float>(fg->bearing.x));
        float gy = std::round(pen_y - sg.y_offset - static_cast<float>(fg->bearing.y));

        local_quads.push_back(LocalQuad{
          .pos_min = {gx, gy},
          .pos_max = {gx + static_cast<float>(fg->size.x), gy + static_cast<float>(fg->size.y)},
          .uv_min = fg->uv_start,
          .uv_max = fg->uv_end,
          .color = sg.style.color,
        });
      }

      if (sg.style.underline && adv_x > 0.0f) {
        float uy = std::round(pen_y - metrics.underline_position);
        float uh = std::max(1.0f, std::round(metrics.underline_thickness));
        local_quads.push_back(LocalQuad{
          .pos_min = {std::floor(pen_x), uy},
          .pos_max = {std::ceil(pen_x + adv_x), uy + uh},
          .uv_min = white_uv,
          .uv_max = white_uv,
          .color = sg.style.color,
        });
      }

      if (sg.style.strikethrough && adv_x > 0.0f) {
        float sy = std::round(pen_y - metrics.strikeout_position);
        float sh = std::max(1.0f, std::round(metrics.strikeout_thickness));
        local_quads.push_back(LocalQuad{
          .pos_min = {std::floor(pen_x), sy},
          .pos_max = {std::ceil(pen_x + adv_x), sy + sh},
          .uv_min = white_uv,
          .uv_max = white_uv,
          .color = sg.style.color,
        });
      }

      pen_x += adv_x;
      pen_y -= sg.y_advance;
    }

    pen_y += metrics.line_height + line_spacing;
  }
}

std::span<const zincgui::Vertex> zincgui::Label::get_vertices() {
  if (ui.get_font_count() == 0) {
    vertices.clear();
    return vertices;
  }

  size_t active_font_idx = font_index < ui.get_font_count() ? font_index : 0;
  const FontFace& font = ui.get_font(active_font_idx);

  vec2i cur_size{width, height};
  if (shape_dirty || cur_size != last_layout_size || font.get_atlas_version() != last_atlas_version) { update_text(); }

  vec2i cur_screen_pos = get_position(Anchor::TOP_LEFT);
  vec2i cur_win_size = ui.get_window_size();
  if (vertices_dirty || cur_screen_pos != last_screen_pos || cur_win_size != last_window_size) {
    rebuild_screen_vertices();
  }

  return vertices;
}

void zincgui::Label::rebuild_screen_vertices() {
  vertices_dirty = false;
  last_screen_pos = get_position(Anchor::TOP_LEFT);
  last_window_size = ui.get_window_size();

  vertices.clear();
  if (local_quads.empty() || last_window_size.x <= 0 || last_window_size.y <= 0) { return; }

  vertices.reserve(local_quads.size() * 6);
  vec2f inv_win{2.0f / static_cast<float>(last_window_size.x), 2.0f / static_cast<float>(last_window_size.y)};
  vec2f origin{static_cast<float>(last_screen_pos.x), static_cast<float>(last_screen_pos.y)};

  for (const auto& q : local_quads) {
    vec2f start = (origin + q.pos_min) * inv_win - vec2f{1.0f, 1.0f};
    vec2f end = (origin + q.pos_max) * inv_win - vec2f{1.0f, 1.0f};

    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, end.y},
                                 .uv = {q.uv_min.x, q.uv_max.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});
    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, end.y},
                                 .uv = {q.uv_max.x, q.uv_max.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});
    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, start.y},
                                 .uv = {q.uv_min.x, q.uv_min.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});

    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, end.y},
                                 .uv = {q.uv_max.x, q.uv_max.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});
    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, start.y},
                                 .uv = {q.uv_max.x, q.uv_min.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});
    vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, start.y},
                                 .uv = {q.uv_min.x, q.uv_min.y},
                                 .atlas_i = (u32)font_index,
                                 .color = q.color});
  }
}
