#include "font_face.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include <glad/glad.h>
#include "common/debug.hpp"
#include "common/logger.hpp"

namespace zincgui {

  FontFace::FontFace(FT_Library freetype_lib, const std::string& path) {
    if (FT_New_Face(freetype_lib, path.c_str(), 0, &m_ft_face)) {
      out::critical("failed to load font at {}", path);
      std::exit(1);
    }
    m_hb_buffer = hb_buffer_create();
    init_atlas(1024, 1024);
  }

  FontFace::FontFace(FT_Library freetype_lib, const void* data, size_t data_size) {
    m_memory_buffer.resize(data_size);
    std::memcpy(m_memory_buffer.data(), data, data_size);

    if (FT_New_Memory_Face(freetype_lib, m_memory_buffer.data(), static_cast<FT_Long>(data_size), 0, &m_ft_face)) {
      out::critical("failed to load font from memory");
      std::exit(1);
    }
    m_hb_buffer = hb_buffer_create();
    init_atlas(1024, 1024);
  }

  FontFace::~FontFace() { cleanup(); }

  FontFace::FontFace(FontFace&& other) noexcept
    : m_ft_face(other.m_ft_face), m_memory_buffer(std::move(other.m_memory_buffer)), m_hb_buffer(other.m_hb_buffer),
      m_texture(other.m_texture), m_sampler(other.m_sampler), m_atlas_width(other.m_atlas_width),
      m_atlas_height(other.m_atlas_height), m_shelf_x(other.m_shelf_x), m_shelf_y(other.m_shelf_y),
      m_shelf_height(other.m_shelf_height), m_white_uv(other.m_white_uv), m_atlas_version(other.m_atlas_version),
      m_atlas_pixels(std::move(other.m_atlas_pixels)), m_metrics_cache(std::move(other.m_metrics_cache)),
      m_glyph_map(std::move(other.m_glyph_map)) {
    other.m_ft_face = nullptr;
    other.m_hb_buffer = nullptr;
    other.m_texture = 0;
    other.m_sampler = 0;
  }

  FontFace& FontFace::operator=(FontFace&& other) noexcept {
    if (this != &other) {
      cleanup();
      m_ft_face = other.m_ft_face;
      m_memory_buffer = std::move(other.m_memory_buffer);
      m_hb_buffer = other.m_hb_buffer;
      m_texture = other.m_texture;
      m_sampler = other.m_sampler;
      m_atlas_width = other.m_atlas_width;
      m_atlas_height = other.m_atlas_height;
      m_shelf_x = other.m_shelf_x;
      m_shelf_y = other.m_shelf_y;
      m_shelf_height = other.m_shelf_height;
      m_white_uv = other.m_white_uv;
      m_atlas_version = other.m_atlas_version;
      m_atlas_pixels = std::move(other.m_atlas_pixels);
      m_metrics_cache = std::move(other.m_metrics_cache);
      m_glyph_map = std::move(other.m_glyph_map);

      other.m_ft_face = nullptr;
      other.m_hb_buffer = nullptr;
      other.m_texture = 0;
      other.m_sampler = 0;
    }
    return *this;
  }

  void FontFace::cleanup() {
    for (auto& [size, m] : m_metrics_cache) {
      if (m.hb_font != nullptr) { hb_font_destroy(m.hb_font); }
      if (m.ft_size != nullptr) { FT_Done_Size(m.ft_size); }
    }
    m_metrics_cache.clear();

    if (m_hb_buffer != nullptr) {
      hb_buffer_destroy(m_hb_buffer);
      m_hb_buffer = nullptr;
    }
    if (m_ft_face != nullptr) {
      FT_Done_Face(m_ft_face);
      m_ft_face = nullptr;
    }
    if (m_texture != 0) {
      glDeleteTextures(1, &m_texture);
      m_texture = 0;
    }
    if (m_sampler != 0) {
      glDeleteSamplers(1, &m_sampler);
      m_sampler = 0;
    }
  }

  void FontFace::init_atlas(i32 width, i32 height) {
    m_atlas_width = width;
    m_atlas_height = height;
    m_atlas_pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);

    for (i32 y = 0; y < 4; y += 1) {
      for (i32 x = 0; x < 4; x += 1) {
        m_atlas_pixels[(static_cast<size_t>(y) * static_cast<size_t>(width)) + static_cast<size_t>(x)] = 255;
      }
    }

    m_shelf_x = 5;
    m_shelf_y = 0;
    m_shelf_height = 5;
    m_white_uv = vec2f{2.0f / static_cast<float>(width), 2.0f / static_cast<float>(height)};

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, m_atlas_pixels.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCreateSamplers(1, &m_sampler);
    glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  bool FontFace::grow_atlas() const {
    i32 new_w = m_atlas_width;
    i32 new_h = m_atlas_height * 2;
    if (new_h > 4096) {
      if (new_w < 4096) {
        new_w = 4096;
        new_h = 4096;
      } else {
        return false;
      }
    }

    std::vector<u8> new_pixels(static_cast<size_t>(new_w) * static_cast<size_t>(new_h), 0);
    for (i32 y = 0; y < m_atlas_height; y += 1) {
      std::memcpy(new_pixels.data() + (static_cast<size_t>(y) * static_cast<size_t>(new_w)),
                  m_atlas_pixels.data() + (static_cast<size_t>(y) * static_cast<size_t>(m_atlas_width)),
                  static_cast<size_t>(m_atlas_width));
    }

    float scale_x = static_cast<float>(m_atlas_width) / static_cast<float>(new_w);
    float scale_y = static_cast<float>(m_atlas_height) / static_cast<float>(new_h);
    for (auto& [k, g] : m_glyph_map) {
      g.uv_start.x *= scale_x;
      g.uv_start.y *= scale_y;
      g.uv_end.x *= scale_x;
      g.uv_end.y *= scale_y;
    }

    m_atlas_width = new_w;
    m_atlas_height = new_h;
    m_atlas_pixels = std::move(new_pixels);
    m_white_uv = vec2f{2.0f / static_cast<float>(new_w), 2.0f / static_cast<float>(new_h)};
    m_atlas_version += 1;

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, new_w, new_h, 0, GL_RED, GL_UNSIGNED_BYTE, m_atlas_pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
  }

  void FontFace::bind(u32 slot) const {
    ensure(m_texture != 0);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glBindSampler(slot, m_sampler);
  }

  const FontSizeMetrics& FontFace::get_metrics(i32 pixel_height) const {
    pixel_height = std::clamp(pixel_height, 4, 256);
    auto it = m_metrics_cache.find(pixel_height);
    if (it != m_metrics_cache.end()) {
      FT_Activate_Size(it->second.ft_size);
      return it->second;
    }

    FontSizeMetrics m{};
    FT_New_Size(m_ft_face, &m.ft_size);
    FT_Activate_Size(m.ft_size);
    FT_Set_Pixel_Sizes(m_ft_face, 0, static_cast<FT_UInt>(pixel_height));

    m.hb_font = hb_ft_font_create(m_ft_face, nullptr);
    hb_ft_font_set_funcs(m.hb_font);

    m.ascender = static_cast<float>(m_ft_face->size->metrics.ascender) / 64.0f;
    m.descender = static_cast<float>(m_ft_face->size->metrics.descender) / 64.0f;
    m.line_height = static_cast<float>(m_ft_face->size->metrics.height) / 64.0f;

    FT_Fixed y_scale = m_ft_face->size->metrics.y_scale;
    if (FT_IS_SCALABLE(m_ft_face)) {
      m.underline_position = static_cast<float>(FT_MulFix(m_ft_face->underline_position, y_scale)) / 64.0f;
      m.underline_thickness =
        std::max(1.0f, static_cast<float>(FT_MulFix(m_ft_face->underline_thickness, y_scale)) / 64.0f);
    } else {
      m.underline_position = -2.0f;
      m.underline_thickness = 1.0f;
    }

    auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_ft_face, FT_SFNT_OS2));
    if (os2 != nullptr && FT_IS_SCALABLE(m_ft_face)) {
      m.strikeout_position = static_cast<float>(FT_MulFix(os2->yStrikeoutPosition, y_scale)) / 64.0f;
      m.strikeout_thickness = std::max(1.0f, static_cast<float>(FT_MulFix(os2->yStrikeoutSize, y_scale)) / 64.0f);
    } else {
      m.strikeout_position = m.ascender * 0.35f;
      m.strikeout_thickness = m.underline_thickness;
    }

    auto [ins_it, _] = m_metrics_cache.emplace(pixel_height, m);
    return ins_it->second;
  }

  const FontGlyph* FontFace::get_glyph(u32 glyph_index, i32 pixel_height, bool bold, bool italic) const {
    pixel_height = std::clamp(pixel_height, 4, 256);
    u64 key = make_glyph_key(glyph_index, pixel_height, bold, italic);
    auto it = m_glyph_map.find(key);
    if (it != m_glyph_map.end()) { return &it->second; }
    return rasterize_glyph(key, glyph_index, pixel_height, bold, italic);
  }

  u32 FontFace::get_codepoint_glyph_index(u32 codepoint) const {
    if (m_ft_face == nullptr) { return 0; }
    return FT_Get_Char_Index(m_ft_face, codepoint);
  }

  hb_buffer_t* FontFace::acquire_hb_buffer() const {
    hb_buffer_clear_contents(m_hb_buffer);
    return m_hb_buffer;
  }

  const FontGlyph* FontFace::rasterize_glyph(u64 key, u32 glyph_index, i32 pixel_height, bool bold, bool italic) const {
    get_metrics(pixel_height);

    if (FT_Load_Glyph(m_ft_face, glyph_index, FT_LOAD_DEFAULT)) { return nullptr; }

    if (m_ft_face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
      if (bold) {
        FT_Pos strength = FT_MulFix(m_ft_face->units_per_EM, m_ft_face->size->metrics.y_scale) / 28;
        FT_Outline_Embolden(&m_ft_face->glyph->outline, strength);
      }
      if (italic) {
        FT_Matrix shear{0x10000L, 0x0366AL, 0, 0x10000L};
        FT_Outline_Transform(&m_ft_face->glyph->outline, &shear);
      }
    }

    if (FT_Render_Glyph(m_ft_face->glyph, FT_RENDER_MODE_NORMAL)) { return nullptr; }

    const FT_Bitmap& bmp = m_ft_face->glyph->bitmap;
    i32 bw = static_cast<i32>(bmp.width);
    i32 bh = static_cast<i32>(bmp.rows);

    if (bw <= 0 || bh <= 0 || bmp.buffer == nullptr) {
      FontGlyph empty_glyph{
        .size = {0, 0},
        .bearing = {m_ft_face->glyph->bitmap_left, m_ft_face->glyph->bitmap_top},
        .uv_start = {0.0f, 0.0f},
        .uv_end = {0.0f, 0.0f},
      };
      auto [it, _] = m_glyph_map.emplace(key, empty_glyph);
      return &it->second;
    }

    if (m_shelf_x + bw + 1 >= m_atlas_width) {
      m_shelf_x = 0;
      m_shelf_y += m_shelf_height + 1;
      m_shelf_height = 0;
    }

    while (m_shelf_y + bh + 1 >= m_atlas_height) {
      if (!grow_atlas()) { return nullptr; }
    }

    i32 pitch = std::abs(bmp.pitch);
    for (i32 row = 0; row < bh; row += 1) {
      const u8* src_row = bmp.buffer + (static_cast<size_t>(row) * static_cast<size_t>(pitch));
      u8* dst_row = m_atlas_pixels.data() +
                    (static_cast<size_t>(m_shelf_y + row) * static_cast<size_t>(m_atlas_width)) +
                    static_cast<size_t>(m_shelf_x);

      if (bmp.pixel_mode == FT_PIXEL_MODE_MONO) {
        for (i32 col = 0; col < bw; col += 1) {
          u8 byte_val = src_row[col >> 3];
          u8 bit_mask = static_cast<u8>(0x80 >> (col & 7));
          dst_row[col] = (byte_val & bit_mask) ? 255 : 0;
        }
      } else {
        std::memcpy(dst_row, src_row, static_cast<size_t>(bw));
      }

      glBindTexture(GL_TEXTURE_2D, m_texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, m_shelf_x, m_shelf_y + row, bw, 1, GL_RED, GL_UNSIGNED_BYTE, dst_row);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FontGlyph font_glyph{
      .size = {bw, bh},
      .bearing = {m_ft_face->glyph->bitmap_left, m_ft_face->glyph->bitmap_top},
      .uv_start =
        {
          static_cast<float>(m_shelf_x) / static_cast<float>(m_atlas_width),
          static_cast<float>(m_shelf_y) / static_cast<float>(m_atlas_height),
        },
      .uv_end =
        {
          static_cast<float>(m_shelf_x + bw) / static_cast<float>(m_atlas_width),
          static_cast<float>(m_shelf_y + bh) / static_cast<float>(m_atlas_height),
        },
    };

    m_shelf_x += bw + 1;
    m_shelf_height = std::max(m_shelf_height, bh);

    auto [it, _] = m_glyph_map.emplace(key, font_glyph);
    return &it->second;
  }

} // namespace zincgui
