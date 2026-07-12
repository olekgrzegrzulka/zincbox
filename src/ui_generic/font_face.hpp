#pragma once
#include <string>
#include <unordered_map>
#include <ft2build.h>
#include <glm/vec2.hpp>
#include "common/types.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftimage.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

struct FontGlyph {
    vec2i size{};
    vec2i bearing{};
    vec2i advance{};
    vec2f uv_start{};
    vec2f uv_end{};
};

class FontFace {
  public:
    FontFace() = default;

    FontFace(FT_Library& freetype_lib, const std::string& path, i32 pixel_height);
    FontFace(FT_Library& freetype_lib, void* data, size_t data_size, i32 pixel_height);

    void bind(u32 slot) const;

    const FontGlyph* find_glyph(u32 charcode) const;

    float get_line_height() const { return m_line_height; }
    float get_ascender() const { return m_ascender; }

  private:
    bool try_creating_glyph_data(FT_Face&, i32 texture_dimensions, i32 pixel_height);

  private:
    float m_line_height = 0.0f;
    float m_ascender = 0.0f;
    u32 texture = 0;
    u32 sampler = 0;
    std::unordered_map<u32, FontGlyph> glyph_map;
};
