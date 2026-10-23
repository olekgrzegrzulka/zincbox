#pragma once
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H
#include <glm/vec2.hpp>
#include <hb.h>
#include "common/types.hpp"
#include <hb-ft.h>

namespace zincgui {

  struct FontGlyph {
      vec2i size{};
      vec2i bearing{};
      vec2f uv_start{};
      vec2f uv_end{};
  };

  struct FontSizeMetrics {
      FT_Size ft_size = nullptr;
      hb_font_t* hb_font = nullptr;
      float ascender = 0.0f;
      float descender = 0.0f;
      float line_height = 0.0f;
      float underline_position = -2.0f;
      float underline_thickness = 1.0f;
      float strikeout_position = 4.0f;
      float strikeout_thickness = 1.0f;
  };

  class FontFace {
    public:
      FontFace() = default;
      FontFace(FT_Library freetype_lib, const std::string& path);
      FontFace(FT_Library freetype_lib, const void* data, size_t data_size);
      ~FontFace();

      FontFace(const FontFace&) = delete;
      FontFace& operator=(const FontFace&) = delete;
      FontFace(FontFace&& other) noexcept;
      FontFace& operator=(FontFace&& other) noexcept;

      void bind(u32 slot) const;

      const FontSizeMetrics& get_metrics(i32 pixel_height) const;
      const FontGlyph* get_glyph(u32 glyph_index, i32 pixel_height, bool bold, bool italic) const;
      u32 get_codepoint_glyph_index(u32 codepoint) const;

      hb_buffer_t* acquire_hb_buffer() const;
      vec2f get_white_uv() const { return m_white_uv; }
      u64 get_atlas_version() const { return m_atlas_version; }

    private:
      void init_atlas(i32 width, i32 height);
      bool grow_atlas() const;
      const FontGlyph* rasterize_glyph(u64 key, u32 glyph_index, i32 pixel_height, bool bold, bool italic) const;
      void cleanup();

      static u64 make_glyph_key(u32 glyph_index, i32 pixel_height, bool bold, bool italic) {
        u64 style = (bold ? 1ULL : 0ULL) | (italic ? 2ULL : 0ULL);
        return (static_cast<u64>(glyph_index) << 32) | (static_cast<u64>(static_cast<u16>(pixel_height)) << 8) | style;
      }

    private:
      FT_Face m_ft_face = nullptr;
      std::vector<u8> m_memory_buffer;
      mutable hb_buffer_t* m_hb_buffer = nullptr;

      mutable u32 m_texture = 0;
      u32 m_sampler = 0;
      mutable i32 m_atlas_width = 0;
      mutable i32 m_atlas_height = 0;
      mutable i32 m_shelf_x = 0;
      mutable i32 m_shelf_y = 0;
      mutable i32 m_shelf_height = 0;
      mutable vec2f m_white_uv{};
      mutable u64 m_atlas_version = 1;
      mutable std::vector<u8> m_atlas_pixels;

      mutable bool m_needs_full_upload = false;
      mutable vec2i m_dirty_min{std::numeric_limits<i32>::max(), std::numeric_limits<i32>::max()};
      mutable vec2i m_dirty_max{0, 0};

      mutable i32 m_active_pixel_height = -1;
      mutable const FontSizeMetrics* m_active_metrics = nullptr;
      mutable std::unordered_map<i32, FontSizeMetrics> m_metrics_cache;
      mutable std::unordered_map<u64, FontGlyph> m_glyph_map;
  };

} // namespace zincgui
