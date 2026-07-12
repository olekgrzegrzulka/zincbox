#include "font_face.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <ft2build.h>
#include <glm/vec2.hpp>
#include "common/debug.hpp"
#include "common/types.hpp"
#include "opengl_includes.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftimage.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

FontFace::FontFace(FT_Library& freetype_lib, const std::string& path, i32 pixel_height) {
  if (FT_New_Face(freetype_lib, path.c_str(), 0, &freetype_face)) {
    out::critical("failed to load font at {}", path);
    exit(1);
  }

  bool success = (try_creating_glyph_data(256, pixel_height) || try_creating_glyph_data(512, pixel_height) ||
                  try_creating_glyph_data(1024, pixel_height));

  if (FT_Done_Face(freetype_face) || !success) {
    out::critical("failed to create font texture atlas of font at {}", path);
    exit(1);
  }
  glCreateSamplers(1, &sampler);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // save_glyph_texture_to_file("./assets/glyphs.png");
}

FontFace::FontFace(FT_Library& freetype_lib, void* data, size_t data_size, i32 pixel_height) {
  if (FT_New_Memory_Face(freetype_lib, static_cast<const FT_Byte*>(data), data_size, 0, &freetype_face)) {
    out::critical("failed to load font");
    exit(1);
  }

  bool success = (try_creating_glyph_data(256, pixel_height) || try_creating_glyph_data(512, pixel_height) ||
                  try_creating_glyph_data(1024, pixel_height));

  if (FT_Done_Face(freetype_face) || !success) {
    out::critical("failed to create font texture atlas of font");
    exit(1);
  }
  glCreateSamplers(1, &sampler);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // save_glyph_texture_to_file("./assets/glyphs.png");
}

/*
  [[maybe_unused]] void save_glyph_texture_to_file(const char* filename) const {
    if (texture == 0) { return; }
    i32 width, height;
    i32 channels = 1;
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    std::vector<u8> texture_data(width * height);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, texture_data.data());
    stbi_write_png(filename, width, height, channels, texture_data.data(), width *
  channels);
  }
 */
void FontFace::bind(u32 slot) const {
  ensure(texture != 0);
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindSampler(slot, sampler);
}

const FontGlyph* FontFace::find_glyph(u32 charcode) const {
  auto it = glyph_map.find(charcode);
  if (it == glyph_map.end()) { return nullptr; }
  return &it->second;
}

bool FontFace::try_creating_glyph_data(i32 texture_dimensions, i32 pixel_height) {
  FT_Set_Pixel_Sizes(freetype_face, 0, pixel_height);

  i32 current_x = 0;
  i32 current_y = 0;
  i32 row_height = 0;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glActiveTexture(GL_TEXTURE0);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texture_dimensions, texture_dimensions, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);

  FT_ULong charcode;
  FT_UInt gindex;
  charcode = FT_Get_First_Char(freetype_face, &gindex);
  i32 character_limit = 2048;

  bool fail = false;

  while (gindex != 0 && --character_limit > 0) {
    if (FT_Load_Char(freetype_face, charcode, FT_LOAD_RENDER)) {
      // debug_warn("failed to character from font at ");
      fail = true;
      break;
    }

    i32 glyph_width = freetype_face->glyph->bitmap.width;
    i32 glyph_height = freetype_face->glyph->bitmap.rows;

    // go to the next row
    if (current_x + glyph_width >= texture_dimensions) {
      current_x = 0;
      current_y += row_height;
      row_height = 0;
    }

    // end of texture
    if (current_y + glyph_height >= texture_dimensions) {
      // debug_warn("texture of size ", texture_dimensions, " is too small to fit the font
      // texture atlas");
      fail = true;
      break;
    }

    ensure(current_x >= 0 && current_x + glyph_width < texture_dimensions);
    ensure(current_y >= 0 && current_y + glyph_height < texture_dimensions);

    glTexSubImage2D(GL_TEXTURE_2D, 0, current_x, current_y, glyph_width, glyph_height, GL_RED, GL_UNSIGNED_BYTE,
                    freetype_face->glyph->bitmap.buffer);

    float half_pixel = 0.5f / texture_dimensions;

    FontGlyph font_glyph{
      .size{
        glyph_width,
        glyph_height,
      },
      .bearing{
        freetype_face->glyph->bitmap_left,
        freetype_face->glyph->bitmap_top,
      },
      .advance{freetype_face->glyph->advance.x, freetype_face->glyph->advance.y},
      .uv_start{
        (current_x + half_pixel) / (float)texture_dimensions,
        (current_y + half_pixel) / (float)texture_dimensions,
      },
      .uv_end{
        (current_x + glyph_width + half_pixel) / (float)texture_dimensions,
        (current_y + glyph_height + half_pixel) / (float)texture_dimensions,
      },
    };

    glyph_map[charcode] = font_glyph;

    current_x += glyph_width;
    row_height = std::max(glyph_height, row_height);
    charcode = FT_Get_Next_Char(freetype_face, charcode, &gindex);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  if (fail) {
    glDeleteTextures(1, &texture);
    texture = 0;
    glyph_map.clear();
    return false;
  }

  return true;
}
