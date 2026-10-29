#include "ui/zincgui/label.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <harfbuzz/hb.h>
#include "common/types.hpp"
#include "core/settings.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/font_face.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/vertex.hpp"
#include "zincbox.hpp"

namespace {
  using namespace zincgui;

  struct TextStyle {
      vec3f color{1.0f};
      bool bold = false;
      bool italic = false;
      bool underline = false;
      bool strikethrough = false;
  };

  struct StyleRun {
      size_t byte_offset = 0;
      size_t byte_length = 0;
      TextStyle style{};
  };

  struct ShapedGlyph {
      u32 glyph_index = 0;
      float x_advance = 0.0f;
      float y_advance = 0.0f;
      float x_offset = 0.0f;
      float y_offset = 0.0f;
      bool is_space = false;
      TextStyle style{};
      const FontGlyph* cached_glyph = nullptr;
  };

  struct ShapedLine {
      std::vector<ShapedGlyph> glyphs;
      float width = 0.0f;
      i32 space_count = 0;
      bool is_last_in_paragraph = false;
  };

  std::optional<vec3f> parse_hex_color(std::string_view hex) {
    if (hex.size() == 7 && hex[0] == '#') {
      auto hex_val = [](char c) -> i32 {
        if (c >= '0' && c <= '9') { return c - '0'; }
        if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
        if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
        return -1;
      };
      i32 r1 = hex_val(hex[1]), r2 = hex_val(hex[2]);
      i32 g1 = hex_val(hex[3]), g2 = hex_val(hex[4]);
      i32 b1 = hex_val(hex[5]), b2 = hex_val(hex[6]);
      if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
        return vec3f{static_cast<float>((r1 << 4) | r2) / 255.0f, static_cast<float>((g1 << 4) | g2) / 255.0f,
                     static_cast<float>((b1 << 4) | b2) / 255.0f};
      }
    }
    return std::nullopt;
  }

  std::string_view parse_paragraph_runs(std::string_view raw, bool parse_tags, const vec3f& base_color,
                                       i32& bold_depth, i32& italic_depth, i32& underline_depth, i32& strike_depth,
                                       std::vector<vec3f>& color_stack, std::string& clean_scratch,
                                       std::vector<StyleRun>& runs) {
    clean_scratch.clear();
    runs.clear();

    if (!parse_tags) {
      TextStyle current_style{
        .color = color_stack.empty() ? base_color : color_stack.back(),
        .bold = bold_depth > 0,
        .italic = italic_depth > 0,
        .underline = underline_depth > 0,
        .strikethrough = strike_depth > 0,
      };
      runs.push_back(StyleRun{0, raw.size(), current_style});
      return raw;
    }

    clean_scratch.reserve(raw.size());
    size_t i = 0;
    size_t run_start_clean = 0;

    auto current_style = [&]() -> TextStyle {
      return TextStyle{
        .color = color_stack.empty() ? base_color : color_stack.back(),
        .bold = bold_depth > 0,
        .italic = italic_depth > 0,
        .underline = underline_depth > 0,
        .strikethrough = strike_depth > 0,
      };
    };

    auto flush_run = [&]() {
      if (clean_scratch.size() > run_start_clean) {
        runs.push_back(StyleRun{
          .byte_offset = run_start_clean,
          .byte_length = clean_scratch.size() - run_start_clean,
          .style = current_style(),
        });
        run_start_clean = clean_scratch.size();
      }
    };

    while (i < raw.size()) {
      if (raw[i] == '<') {
        size_t close = raw.find('>', i);
        if (close != std::string_view::npos) {
          std::string_view tag = raw.substr(i + 1, close - i - 1);
          bool is_open = true;
          if (!tag.empty() && tag[0] == '/') {
            is_open = false;
            tag = tag.substr(1);
          }

          bool recognized = false;
          if (tag == "b") {
            flush_run();
            bold_depth += is_open ? 1 : -1;
            if (bold_depth < 0) { bold_depth = 0; }
            recognized = true;
          } else if (tag == "i") {
            flush_run();
            italic_depth += is_open ? 1 : -1;
            if (italic_depth < 0) { italic_depth = 0; }
            recognized = true;
          } else if (tag == "u") {
            flush_run();
            underline_depth += is_open ? 1 : -1;
            if (underline_depth < 0) { underline_depth = 0; }
            recognized = true;
          } else if (tag == "s") {
            flush_run();
            strike_depth += is_open ? 1 : -1;
            if (strike_depth < 0) { strike_depth = 0; }
            recognized = true;
          } else if (is_open && tag.starts_with("color=") && tag.size() > 6) {
            auto hex = tag.substr(6);
            if (hex.size() >= 2 && hex.front() == '"' && hex.back() == '"') { hex = hex.substr(1, hex.size() - 2); }
            auto c = parse_hex_color(hex);
            if (c) {
              flush_run();
              color_stack.push_back(*c);
              recognized = true;
            }
          } else if (!is_open && tag == "color") {
            if (color_stack.size() > 1) {
              flush_run();
              color_stack.pop_back();
            }
            recognized = true;
          }

          if (recognized) {
            i = close + 1;
            continue;
          }
        }
      }

      clean_scratch.push_back(raw[i]);
      i += 1;
    }

    flush_run();
    if (runs.empty()) { runs.push_back(StyleRun{0, 0, current_style()}); }
    return clean_scratch;
  }

} // namespace

zincgui::Label::Label(Root& ui_) : Widget::Widget(ui_) {
  text_color(theme::config().text_color);
  clip(true);
}

zincgui::Label::Label(Root& ui_, std::string_view text_) : Widget::Widget(ui_) {
  text_color(theme::config().text_color);
  text(text_);
  update_text();
  clip(true);
}

zincgui::Label::~Label() = default;

void zincgui::Label::update() { Widget::update(); }

std::span<const zincgui::Vertex> zincgui::Label::vertices() {
  if (ui().get_font_count() == 0) {
    m_vertices.clear();
    return m_vertices;
  }

  size_t active_font_idx = m_font_index < ui().get_font_count() ? m_font_index : 0;
  const FontFace& font = ui().get_font(active_font_idx);

  vec2i cur_size{width(), height()};
  if (m_shape_dirty || cur_size != m_last_layout_size || font.get_atlas_version() != m_last_atlas_version) {
    update_text();
  }

  vec2i cur_screen_pos = position(Anchor::TOP_LEFT);
  vec2i cur_win_size = ui().get_window_size();
  if (m_vertices_dirty || cur_screen_pos != m_last_screen_pos || cur_win_size != m_last_window_size) {
    rebuild_screen_vertices();
  }

  return m_vertices;
}

void zincgui::Label::font(size_t index) {
  if (m_font_index == index) { return; }
  m_font_index = index;
  m_shape_dirty = true;
  m_last_atlas_version = static_cast<u64>(-1);
  mark_dirty();
}

void zincgui::Label::font(std::string_view name) {
  auto id_opt = ui().find_font_id(name);
  if (id_opt) { font(*id_opt); }
}

i32 zincgui::Label::font_size() const {
  if (m_font_size > 0) { return m_font_size; }
  if (zincbox::font_size() > 0) { return zincbox::font_size(); }
  return ui().get_default_font_size();
}

zincgui::Label& zincgui::Label::resize_to_text_extents(bool to) {
  if (to != m_resize_to_text_extents) {
    m_resize_to_text_extents = to;
    m_shape_dirty = true;
    mark_dirty();
  }
  if (m_resize_to_text_extents && (!parent() || !parent()->has_layout())) {
    if (m_shape_dirty) { update_text(); }
    width(static_cast<i32>(m_text_extents.x))
      .height(static_cast<i32>(m_text_extents.y))
      .min_width(static_cast<i32>(m_text_extents.x))
      .min_height(static_cast<i32>(m_text_extents.y));
  }
  return *this;
}

void zincgui::Label::update_text() {
  m_shape_dirty = false;
  m_vertices_dirty = true;
  m_local_quads.clear();
  m_text_extents = {};

  if (ui().get_font_count() == 0 || m_text.empty()) {
    m_last_layout_size = vec2i{width(), height()};
    return;
  }

  size_t active_font_idx = m_font_index < ui().get_font_count() ? m_font_index : 0;
  const FontFace& font = ui().get_font(active_font_idx);
  i32 px_size = font_size();
  const FontSizeMetrics& metrics = font.get_metrics(px_size);
  hb_buffer_t* hb_buf = font.acquire_hb_buffer();

  std::vector<ShapedLine> lines;
  i32 bold_depth = 0;
  i32 italic_depth = 0;
  i32 underline_depth = 0;
  i32 strike_depth = 0;
  std::vector<vec3f> color_stack;
  color_stack.push_back(m_text_color);

  std::string clean_paragraph_scratch;
  std::vector<StyleRun> paragraph_runs;

  size_t line_start = 0;
  while (line_start <= m_text.size()) {
    size_t next_nl = m_text.find('\n', line_start);
    std::string_view raw_para = (next_nl == std::string::npos)
                                  ? std::string_view(m_text).substr(line_start)
                                  : std::string_view(m_text).substr(line_start, next_nl - line_start);

    std::string_view para_view =
      parse_paragraph_runs(raw_para, m_parse_tags, m_text_color, bold_depth, italic_depth, underline_depth, strike_depth,
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

    if (m_word_wrap && width() > 0 && !para_glyphs.empty()) {
      float max_w = static_cast<float>(width());
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

  if (m_ellipsis && !m_word_wrap && width() > 0) {
    u32 dot_glyph = font.get_codepoint_glyph_index('.');
    const FontGlyph* dot_fg = font.get_glyph(dot_glyph, px_size, false, false);
    float dot_adv = dot_fg ? static_cast<float>(dot_fg->size.x + 1) : 4.0f;
    float ellipsis_w = dot_adv * 3.0f;
    float max_w = static_cast<float>(width());

    for (auto& line : lines) {
      if (line.width > max_w && max_w > ellipsis_w) {
        float accum = 0.0f;
        size_t keep = 0;
        TextStyle last_style{.color = m_text_color};
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
    m_text_extents.x = std::max(m_text_extents.x, std::ceil(line.width));
  }
  i32 lines_count = std::max<i32>(1, static_cast<i32>(lines.size()));
  m_text_extents.y = std::ceil((metrics.line_height * static_cast<float>(lines_count)) +
                             (m_line_spacing * static_cast<float>(lines_count - 1)));

  if (m_resize_to_text_extents && (!parent() || !parent()->has_layout())) {
    width(static_cast<i32>(m_text_extents.x))
      .height(static_cast<i32>(m_text_extents.y))
      .min_width(static_cast<i32>(m_text_extents.x))
      .min_height(static_cast<i32>(m_text_extents.y));
  }

  vec2f anchor_uv = anchor_to_uv(m_label_anchor);
  TextAlign effective_align = m_text_alignment;
  if (!m_custom_alignment_set) {
    if (anchor_uv.x < 0.25f) {
      effective_align = TextAlign::LEFT;
    } else if (anchor_uv.x > 0.75f) {
      effective_align = TextAlign::RIGHT;
    } else {
      effective_align = TextAlign::CENTER;
    }
  }

  float block_offset_y = std::round(anchor_uv.y * (static_cast<float>(height()) - m_text_extents.y));
  float target_box_width = static_cast<float>(width()) > 0.0f ? static_cast<float>(width()) : m_text_extents.x;

  u64 ver_before = font.get_atlas_version();
  size_t total_glyph_estimate = 0;
  for (auto& line : lines) {
    total_glyph_estimate += line.glyphs.size();
    for (auto& sg : line.glyphs) {
      sg.cached_glyph = font.get_glyph(sg.glyph_index, px_size, sg.style.bold, sg.style.italic);
    }
  }

  m_last_atlas_version = font.get_atlas_version();
  bool atlas_regrew = (m_last_atlas_version != ver_before);
  m_last_layout_size = vec2i{width(), height()};
  vec2f white_uv = font.get_white_uv();
  m_local_quads.reserve(total_glyph_estimate);

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

        m_local_quads.push_back(LocalQuad{
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
        m_local_quads.push_back(LocalQuad{
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
        m_local_quads.push_back(LocalQuad{
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

    pen_y += metrics.line_height + m_line_spacing;
  }
}

void zincgui::Label::rebuild_screen_vertices() {
  m_vertices_dirty = false;
  m_last_screen_pos = position(Anchor::TOP_LEFT);
  m_last_window_size = ui().get_window_size();

  m_vertices.clear();
  if (m_local_quads.empty() || m_last_window_size.x <= 0 || m_last_window_size.y <= 0) { return; }

  m_vertices.reserve(m_local_quads.size() * 6);
  vec2f inv_win{2.0f / static_cast<float>(m_last_window_size.x), 2.0f / static_cast<float>(m_last_window_size.y)};
  vec2f origin{static_cast<float>(m_last_screen_pos.x), static_cast<float>(m_last_screen_pos.y)};

  for (const auto& q : m_local_quads) {
    vec2f start = (origin + q.pos_min) * inv_win - vec2f{1.0f, 1.0f};
    vec2f end = (origin + q.pos_max) * inv_win - vec2f{1.0f, 1.0f};

    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, end.y},
                                 .uv = {q.uv_min.x, q.uv_max.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});
    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, end.y},
                                 .uv = {q.uv_max.x, q.uv_max.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});
    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, start.y},
                                 .uv = {q.uv_min.x, q.uv_min.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});

    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, end.y},
                                 .uv = {q.uv_max.x, q.uv_max.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});
    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {end.x, start.y},
                                 .uv = {q.uv_max.x, q.uv_min.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});
    m_vertices.emplace_back(Vertex{.type = VertexType::LABEL,
                                 .pos = {start.x, start.y},
                                 .uv = {q.uv_min.x, q.uv_min.y},
                                 .atlas_i = (u32)m_font_index,
                                 .color = q.color});
  }
}
