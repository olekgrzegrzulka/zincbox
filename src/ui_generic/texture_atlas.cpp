#include <cassert>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <glm/vec3.hpp>
#include "common/debug.hpp"
#include "common/types.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_write.h"
#include "opengl_includes.hpp"
#include "texture_atlas.hpp"

TextureAtlas::TextureAtlas(i32 atlas_size_, i32 margin_px_, i32 min_texture_size_px_) {
  atlas_size = atlas_size_;
  margin_px = margin_px_;
  min_texture_size_px = min_texture_size_px_;
  image.resize(4 * atlas_size * atlas_size);
  free_16x16_squares.resize((atlas_size / min_texture_size_px) * (atlas_size / min_texture_size_px), true);
}

bool TextureAtlas::add_texture(std::string id, std::string path) {
  if (textures.contains(id)) { return false; }

  i32 width, height, channels;
  stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!data) {
    debug_error("failed to load texture" + path);
  }

  auto at = paste_texture(data, width, height);

  TextureAtlasData uv;
  uv.start = {at.x / (float)atlas_size - half_pixel(), at.y / (float)atlas_size - half_pixel()};
  uv.end = {(at.x + width) / (float)atlas_size + half_pixel(), (at.y + height) / (float)atlas_size + half_pixel()};
  uv.width = width;
  uv.height = height;
  textures[id] = uv;

  stbi_image_free(data);

  return true;
}

bool TextureAtlas::add_texture(std::string id, const std::vector<u8>& data_, i32 width, i32 height) {
  if (textures.contains(id)) { return false; }
  stbi_uc* data = (stbi_uc*)data_.data();
  if (!data) {
    debug_warn("failed to load texture from data");
    return false;
  }

  auto at = paste_texture(data, width, height);

  TextureAtlasData uv;
  uv.start = {at.x / (float)atlas_size - half_pixel(), at.y / (float)atlas_size - half_pixel()};
  uv.end = {(at.x + width) / (float)atlas_size + half_pixel(), (at.y + height) / (float)atlas_size + half_pixel()};
  uv.width = width;
  uv.height = height;
  textures[id] = uv;

  // stbi_image_free(data);

  return true;
}

void TextureAtlas::save_to_file(std::string filename) {
  if (!filename.ends_with(".png")) { filename += ".png"; }
  i32 status = stbi_write_png(filename.c_str(), atlas_size, atlas_size, 4, image.data(), 4 * atlas_size);
  if (status == 1) {
    debug_log("Saved texture atlas to ", filename);
  } else {
    debug_warn("Couldn't save texture atlas to ", filename);
  }
}

std::optional<std::reference_wrapper<TextureAtlasData>> TextureAtlas::get(std::string id) {
  auto it = textures.find(id);
  if (it != textures.end()) {
    return it->second;
  } else if (fallback_texture.has_value()) {
    return get(fallback_texture.value());
  } else {
    return std::nullopt;
  }
}

void TextureAtlas::bind(u32 slot) {
  if (dirty) { regenerate_texture(); }
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindSampler(slot, sampler);
}

void TextureAtlas::set_fallback_texture(std::string id) {
  if (!has_texture(id)) { return; }
  fallback_texture = id;
}

void TextureAtlas::regenerate_texture() {
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, atlas_size, atlas_size);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlas_size, atlas_size, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glCreateSamplers(1, &sampler);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  dirty = false;
}

std::optional<std::pair<i32, i32>> TextureAtlas::find_space_for_texture(i32 width, i32 height) {
  for (i32 square_x = 0; square_x < (atlas_size / min_texture_size_px); square_x += 1) {
    for (i32 square_y = 0; square_y < (atlas_size / min_texture_size_px); square_y += 1) {
      bool fail = false;
      for (i32 square_width = 0; square_width < std::ceil(width / (double)min_texture_size_px); square_width += 1) {
        for (i32 square_height = 0; square_height < std::ceil(height / (double)min_texture_size_px); square_height += 1) {
          size_t square_i = (atlas_size / min_texture_size_px) * (square_y + square_height) + (square_x + square_width);
          // size_t square_i = square_x + square_y * (atlas_size / square_size_px);
          if (square_x + square_width >= (atlas_size / min_texture_size_px) || square_y + square_height >= (atlas_size / min_texture_size_px)) {
            fail = true;
            break;
          }
          if (square_i >= free_16x16_squares.size()) {
            debug_log(square_i);
          }
          if (!free_16x16_squares[square_i]) {
            fail = true;
            break;
          }
        }
        if (fail) { break; }
      }

      if (!fail) {
        return std::make_pair(square_x * min_texture_size_px, square_y * min_texture_size_px);
      }
    }
  }
  debug_warn("fail");
  return std::nullopt;
}

void TextureAtlas::mark_space_for_texture(i32 x, i32 y, i32 width, i32 height) {
  i32 begin_x = x / min_texture_size_px;
  i32 begin_y = y / min_texture_size_px;
  i32 end_x = begin_x + std::ceil(width / (double)min_texture_size_px);
  i32 end_y = begin_y + std::ceil(height / (double)min_texture_size_px);

  for (i32 square_x = begin_x; square_x < end_x; square_x += 1) {
    for (i32 square_y = begin_y; square_y < end_y; square_y += 1) {
      // size_t square_i = square_x + square_y * (atlas_size / square_size_px);
      ensure(free_16x16_squares[(atlas_size / min_texture_size_px) * square_y + square_x]);
      // ensure(free_16x16_squares[square_i]);
      free_16x16_squares[(atlas_size / min_texture_size_px) * square_y + square_x] = false;
      // ensure(square_i < free_16x16_squares.size());

      // free_16x16_squares[square_i] = false;
    }
  }
}

vec2i TextureAtlas::paste_texture(stbi_uc* data, i32 width, i32 height) {
  auto [x, y] = find_space_for_texture(width + 2 * margin_px, height + 2 * margin_px).value_or(std::make_pair(0, 0));
  mark_space_for_texture(
    x + margin_px, y + margin_px,
    width + 2 * margin_px, height + 2 * margin_px);

  for (i32 oy = 0; oy < height; oy += 1) {
    for (i32 ox = 0; ox < width; ox += 1) {
      if ((x + ox) < atlas_size && (y + oy) < atlas_size) {
        for (i32 c = 0; c < 4; c += 1) {
          image[4 * ((y + oy) * atlas_size + (x + ox)) + c] = data[4 * (oy * width + ox) + c];
        }
      }
    }
  }

  return {x, y};
}
