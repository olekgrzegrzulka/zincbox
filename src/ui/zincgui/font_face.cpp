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
    if (FT_New_Face(freetype_lib, path.c_str(), 0, &ft_face_)) {
      out::critical("failed to load font at {}", path);
      std::exit(1);
    }
    hb_buffer_ = hb_buffer_create();
    init_atlas(1024, 1024);
  }

  FontFace::FontFace(FT_Library freetype_lib, const void* data, size_t data_size) {
    memory_buffer_.resize(data_size);
    std::memcpy(memory_buffer_.data(), data, data_size);

    if (FT_New_Memory_Face(freetype_lib, memory_buffer_.data(), static_cast<FT_Long>(data_size), 0, &ft_face_)) {
      out::critical("failed to load font from memory");
      std::exit(1);
    }
    hb_buffer_ = hb_buffer_create();
    init_atlas(1024, 1024);
  }

  FontFace::~FontFace() { cleanup(); }

  FontFace::FontFace(FontFace&& other) noexcept
    : ft_face_(other.ft_face_), memory_buffer_(std::move(other.memory_buffer_)), hb_buffer_(other.hb_buffer_),
      texture_(other.texture_), sampler_(other.sampler_), atlas_width_(other.atlas_width_),
      atlas_height_(other.atlas_height_), shelf_x_(other.shelf_x_), shelf_y_(other.shelf_y_),
      shelf_height_(other.shelf_height_), white_uv_(other.white_uv_), atlas_version_(other.atlas_version_),
      atlas_pixels_(std::move(other.atlas_pixels_)), metrics_cache_(std::move(other.metrics_cache_)),
      glyph_map_(std::move(other.glyph_map_)) {
    other.ft_face_ = nullptr;
    other.hb_buffer_ = nullptr;
    other.texture_ = 0;
    other.sampler_ = 0;
  }

  FontFace& FontFace::operator=(FontFace&& other) noexcept {
    if (this != &other) {
      cleanup();
      ft_face_ = other.ft_face_;
      memory_buffer_ = std::move(other.memory_buffer_);
      hb_buffer_ = other.hb_buffer_;
      texture_ = other.texture_;
      sampler_ = other.sampler_;
      atlas_width_ = other.atlas_width_;
      atlas_height_ = other.atlas_height_;
      shelf_x_ = other.shelf_x_;
      shelf_y_ = other.shelf_y_;
      shelf_height_ = other.shelf_height_;
      white_uv_ = other.white_uv_;
      atlas_version_ = other.atlas_version_;
      atlas_pixels_ = std::move(other.atlas_pixels_);
      metrics_cache_ = std::move(other.metrics_cache_);
      glyph_map_ = std::move(other.glyph_map_);

      other.ft_face_ = nullptr;
      other.hb_buffer_ = nullptr;
      other.texture_ = 0;
      other.sampler_ = 0;
    }
    return *this;
  }

  void FontFace::cleanup() {
    for (auto& [size, m] : metrics_cache_) {
      if (m.hb_font != nullptr) { hb_font_destroy(m.hb_font); }
      if (m.ft_size != nullptr) { FT_Done_Size(m.ft_size); }
    }
    metrics_cache_.clear();

    if (hb_buffer_ != nullptr) {
      hb_buffer_destroy(hb_buffer_);
      hb_buffer_ = nullptr;
    }
    if (ft_face_ != nullptr) {
      FT_Done_Face(ft_face_);
      ft_face_ = nullptr;
    }
    if (texture_ != 0) {
      glDeleteTextures(1, &texture_);
      texture_ = 0;
    }
    if (sampler_ != 0) {
      glDeleteSamplers(1, &sampler_);
      sampler_ = 0;
    }
  }

  void FontFace::init_atlas(i32 width, i32 height) {
    atlas_width_ = width;
    atlas_height_ = height;
    atlas_pixels_.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);

    for (i32 y = 0; y < 4; ++y) {
      for (i32 x = 0; x < 4; ++x) {
        atlas_pixels_[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = 255;
      }
    }

    shelf_x_ = 5;
    shelf_y_ = 0;
    shelf_height_ = 5;
    white_uv_ = vec2f{2.0f / static_cast<float>(width), 2.0f / static_cast<float>(height)};

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_pixels_.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCreateSamplers(1, &sampler_);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  bool FontFace::grow_atlas() const {
    i32 new_w = atlas_width_;
    i32 new_h = atlas_height_ * 2;
    if (new_h > 4096) {
      if (new_w < 4096) {
        new_w = 4096;
        new_h = 4096;
      } else {
        return false;
      }
    }

    std::vector<u8> new_pixels(static_cast<size_t>(new_w) * static_cast<size_t>(new_h), 0);
    for (i32 y = 0; y < atlas_height_; ++y) {
      std::memcpy(new_pixels.data() + static_cast<size_t>(y) * static_cast<size_t>(new_w),
                  atlas_pixels_.data() + static_cast<size_t>(y) * static_cast<size_t>(atlas_width_),
                  static_cast<size_t>(atlas_width_));
    }

    float scale_x = static_cast<float>(atlas_width_) / static_cast<float>(new_w);
    float scale_y = static_cast<float>(atlas_height_) / static_cast<float>(new_h);
    for (auto& [k, g] : glyph_map_) {
      g.uv_start.x *= scale_x;
      g.uv_start.y *= scale_y;
      g.uv_end.x *= scale_x;
      g.uv_end.y *= scale_y;
    }

    atlas_width_ = new_w;
    atlas_height_ = new_h;
    atlas_pixels_ = std::move(new_pixels);
    white_uv_ = vec2f{2.0f / static_cast<float>(new_w), 2.0f / static_cast<float>(new_h)};
    atlas_version_ += 1;

    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, new_w, new_h, 0, GL_RED, GL_UNSIGNED_BYTE, atlas_pixels_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
  }

  void FontFace::bind(u32 slot) const {
    ensure(texture_ != 0);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindSampler(slot, sampler_);
  }

  const FontSizeMetrics& FontFace::get_metrics(i32 pixel_height) const {
    pixel_height = std::clamp(pixel_height, 4, 256);
    auto it = metrics_cache_.find(pixel_height);
    if (it != metrics_cache_.end()) {
      FT_Activate_Size(it->second.ft_size);
      return it->second;
    }

    FontSizeMetrics m{};
    FT_New_Size(ft_face_, &m.ft_size);
    FT_Activate_Size(m.ft_size);
    FT_Set_Pixel_Sizes(ft_face_, 0, static_cast<FT_UInt>(pixel_height));

    m.hb_font = hb_ft_font_create(ft_face_, nullptr);
    hb_ft_font_set_funcs(m.hb_font);

    m.ascender = static_cast<float>(ft_face_->size->metrics.ascender) / 64.0f;
    m.descender = static_cast<float>(ft_face_->size->metrics.descender) / 64.0f;
    m.line_height = static_cast<float>(ft_face_->size->metrics.height) / 64.0f;

    FT_Fixed y_scale = ft_face_->size->metrics.y_scale;
    if (FT_IS_SCALABLE(ft_face_)) {
      m.underline_position = static_cast<float>(FT_MulFix(ft_face_->underline_position, y_scale)) / 64.0f;
      m.underline_thickness =
        std::max(1.0f, static_cast<float>(FT_MulFix(ft_face_->underline_thickness, y_scale)) / 64.0f);
    } else {
      m.underline_position = -2.0f;
      m.underline_thickness = 1.0f;
    }

    auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(ft_face_, FT_SFNT_OS2));
    if (os2 != nullptr && FT_IS_SCALABLE(ft_face_)) {
      m.strikeout_position = static_cast<float>(FT_MulFix(os2->yStrikeoutPosition, y_scale)) / 64.0f;
      m.strikeout_thickness = std::max(1.0f, static_cast<float>(FT_MulFix(os2->yStrikeoutSize, y_scale)) / 64.0f);
    } else {
      m.strikeout_position = m.ascender * 0.35f;
      m.strikeout_thickness = m.underline_thickness;
    }

    auto [ins_it, _] = metrics_cache_.emplace(pixel_height, m);
    return ins_it->second;
  }

  const FontGlyph* FontFace::get_glyph(u32 glyph_index, i32 pixel_height, bool bold, bool italic) const {
    pixel_height = std::clamp(pixel_height, 4, 256);
    u64 key = make_glyph_key(glyph_index, pixel_height, bold, italic);
    auto it = glyph_map_.find(key);
    if (it != glyph_map_.end()) { return &it->second; }
    return rasterize_glyph(key, glyph_index, pixel_height, bold, italic);
  }

  u32 FontFace::get_codepoint_glyph_index(u32 codepoint) const {
    if (ft_face_ == nullptr) { return 0; }
    return FT_Get_Char_Index(ft_face_, codepoint);
  }

  hb_buffer_t* FontFace::acquire_hb_buffer() const {
    hb_buffer_clear_contents(hb_buffer_);
    return hb_buffer_;
  }

  const FontGlyph* FontFace::rasterize_glyph(u64 key, u32 glyph_index, i32 pixel_height, bool bold, bool italic) const {
    get_metrics(pixel_height);

    if (FT_Load_Glyph(ft_face_, glyph_index, FT_LOAD_DEFAULT)) { return nullptr; }

    if (ft_face_->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
      if (bold) {
        FT_Pos strength = FT_MulFix(ft_face_->units_per_EM, ft_face_->size->metrics.y_scale) / 28;
        FT_Outline_Embolden(&ft_face_->glyph->outline, strength);
      }
      if (italic) {
        FT_Matrix shear{0x10000L, 0x0366AL, 0, 0x10000L};
        FT_Outline_Transform(&ft_face_->glyph->outline, &shear);
      }
    }

    if (FT_Render_Glyph(ft_face_->glyph, FT_RENDER_MODE_NORMAL)) { return nullptr; }

    const FT_Bitmap& bmp = ft_face_->glyph->bitmap;
    i32 bw = static_cast<i32>(bmp.width);
    i32 bh = static_cast<i32>(bmp.rows);

    if (bw <= 0 || bh <= 0 || bmp.buffer == nullptr) {
      FontGlyph empty_glyph{
        .size = {0, 0},
        .bearing = {ft_face_->glyph->bitmap_left, ft_face_->glyph->bitmap_top},
        .uv_start = {0.0f, 0.0f},
        .uv_end = {0.0f, 0.0f},
      };
      auto [it, _] = glyph_map_.emplace(key, empty_glyph);
      return &it->second;
    }

    if (shelf_x_ + bw + 1 >= atlas_width_) {
      shelf_x_ = 0;
      shelf_y_ += shelf_height_ + 1;
      shelf_height_ = 0;
    }

    while (shelf_y_ + bh + 1 >= atlas_height_) {
      if (!grow_atlas()) { return nullptr; }
    }

    i32 pitch = std::abs(bmp.pitch);
    for (i32 row = 0; row < bh; ++row) {
      const u8* src_row = bmp.buffer + static_cast<size_t>(row) * static_cast<size_t>(pitch);
      u8* dst_row = atlas_pixels_.data() + static_cast<size_t>(shelf_y_ + row) * static_cast<size_t>(atlas_width_) +
                    static_cast<size_t>(shelf_x_);

      if (bmp.pixel_mode == FT_PIXEL_MODE_MONO) {
        for (i32 col = 0; col < bw; ++col) {
          u8 byte_val = src_row[col >> 3];
          u8 bit_mask = static_cast<u8>(0x80 >> (col & 7));
          dst_row[col] = (byte_val & bit_mask) ? 255 : 0;
        }
      } else {
        std::memcpy(dst_row, src_row, static_cast<size_t>(bw));
      }

      glBindTexture(GL_TEXTURE_2D, texture_);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, shelf_x_, shelf_y_ + row, bw, 1, GL_RED, GL_UNSIGNED_BYTE, dst_row);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FontGlyph font_glyph{
      .size = {bw, bh},
      .bearing = {ft_face_->glyph->bitmap_left, ft_face_->glyph->bitmap_top},
      .uv_start =
        {
          static_cast<float>(shelf_x_) / static_cast<float>(atlas_width_),
          static_cast<float>(shelf_y_) / static_cast<float>(atlas_height_),
        },
      .uv_end =
        {
          static_cast<float>(shelf_x_ + bw) / static_cast<float>(atlas_width_),
          static_cast<float>(shelf_y_ + bh) / static_cast<float>(atlas_height_),
        },
    };

    shelf_x_ += bw + 1;
    shelf_height_ = std::max(shelf_height_, bh);

    auto [it, _] = glyph_map_.emplace(key, font_glyph);
    return &it->second;
  }

} // namespace zincgui
