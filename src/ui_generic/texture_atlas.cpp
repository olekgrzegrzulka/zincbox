#include <cassert>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <glm/vec3.hpp>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_write.h"
#include "opengl_includes.hpp"
#include "texture_atlas.hpp"

TextureAtlas::TextureAtlas(i32 atlas_size_px_, i32 margin_px_, i32 grid_size_px_) {
  atlas_size_px = atlas_size_px_;
  margin_px = margin_px_;
  grid_size_px = grid_size_px_;
  image.resize((u64)4 * atlas_size_px * atlas_size_px);
  occupied_grid_space.resize((u64)(atlas_size_px / grid_size_px) * (u64)(atlas_size_px / grid_size_px), false);
}

bool TextureAtlas::add_texture(std::string_view id, std::string path) {
  if (textures.contains(id)) { return false; }

  i32 width, height, channels;
  stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!data) {
    out::log_critical("failed to load texture {}", path);
    exit(1);
  }

  auto at = paste_texture(data, width, height);

  TextureAtlasData uv;
  uv.start = {at.x / (float)atlas_size_px - half_pixel(), at.y / (float)atlas_size_px - half_pixel()};
  uv.end = {(at.x + width) / (float)atlas_size_px + half_pixel(),
            (at.y + height) / (float)atlas_size_px + half_pixel()};
  uv.width = width;
  uv.height = height;
  textures[std::string(id)] = uv;

  stbi_image_free(data);

  return true;
}

bool TextureAtlas::add_texture(std::string_view id, const std::vector<u8>& data_, i32 width, i32 height) {
  if (textures.contains(id)) { return false; }
  stbi_uc* data = (stbi_uc*)data_.data();
  if (!data) {
    out::debug_warning("failed to load texture from data");
    return false;
  }

  auto at = paste_texture(data, width, height);

  TextureAtlasData uv;
  uv.start = {at.x / (float)atlas_size_px - half_pixel(), at.y / (float)atlas_size_px - half_pixel()};
  uv.end = {(at.x + width) / (float)atlas_size_px + half_pixel(),
            (at.y + height) / (float)atlas_size_px + half_pixel()};
  uv.width = width;
  uv.height = height;
  textures[std::string(id)] = uv;

  // stbi_image_free(data);

  return true;
}

bool TextureAtlas::add_texture(std::string_view id, const u8* data_, i32 width, i32 height) {
  if (textures.contains(id)) { return false; }
  stbi_uc* data = (stbi_uc*)data_;
  if (!data) {
    out::debug_warning("failed to load texture from data");
    return false;
  }

  auto at = paste_texture(data, width, height);

  TextureAtlasData uv;
  uv.start = {at.x / (float)atlas_size_px - half_pixel(), at.y / (float)atlas_size_px - half_pixel()};
  uv.end = {(at.x + width) / (float)atlas_size_px + half_pixel(),
            (at.y + height) / (float)atlas_size_px + half_pixel()};
  uv.width = width;
  uv.height = height;
  textures[std::string(id)] = uv;

  return true;
}

bool TextureAtlas::add_texture_row(std::span<const std::string> ids, std::string path) {
  for (const auto& id : ids) {
    if (textures.contains(id)) { return false; }
  }

  i32 width, height, channels;
  stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!data) {
    out::log_critical("failed to load texture {}", path);
    exit(1);
  }

  bool res = add_texture_row(ids, data, width, height);
  stbi_image_free(data);
  return res;
}

bool TextureAtlas::add_texture_row(std::span<const std::string> ids, const u8* data, i32 width, i32 height) {
  for (const auto& id : ids) {
    if (textures.contains(id)) { return false; }
  }

  if (!data) {
    out::debug_warning("failed to load texture row from data");
    return false;
  }

  auto at = paste_texture((stbi_uc*)data, width, height);
  i32 frag_width = width / (i32)ids.size();

  for (size_t i = 0; i < ids.size(); i += 1) {
    TextureAtlasData uv;
    i32 frag_x = at.x + (i32)i * frag_width;

    uv.start = {frag_x / (float)atlas_size_px - half_pixel(), at.y / (float)atlas_size_px - half_pixel()};
    uv.end = {(frag_x + frag_width) / (float)atlas_size_px + half_pixel(),
              (at.y + height) / (float)atlas_size_px + half_pixel()};
    uv.width = frag_width;
    uv.height = height;

    textures[ids[i]] = uv;
  }

  return true;
}

void TextureAtlas::add_texture_alias(std::string id, std::string to) { aliases[std::move(id)] = std::move(to); }

bool TextureAtlas::remove_texture(std::string_view id) {
  bool textures_contains_id = textures.contains(id);
  bool aliases_contains_id = aliases.contains(id);
  if (textures_contains_id) {
    std::string id_str(id);
    auto& uv = textures.at(id_str);
    mark_space_for_texture(uv.start.x * atlas_size_px, uv.start.y * atlas_size_px, uv.width, uv.height, true);
    for (i32 y = 0; y < uv.height; y++) {
      for (i32 x = 0; x < uv.width; x++) {
        i32 atlas_x = uv.start.x * atlas_size_px + x;
        i32 atlas_y = uv.start.y * atlas_size_px + y;
        i32 atlas_i = (atlas_y * atlas_size_px + atlas_x) * 4;
        image[atlas_i + 0] = 0;
        image[atlas_i + 1] = 0;
        image[atlas_i + 2] = 0;
        image[atlas_i + 3] = 0;
      }
    }
    dirty = true;
    textures.erase(id_str);
  }
  if (aliases_contains_id) { aliases.erase(std::string(id)); }

  return textures_contains_id || aliases_contains_id;
}

vec2i TextureAtlas::get_texture_size(std::string_view id) {
  auto texture_atlas_data = get(id);
  if (!texture_atlas_data.has_value()) { return {0, 0}; }
  return {texture_atlas_data->get().width, texture_atlas_data->get().height};
}

void TextureAtlas::save_to_file(std::string filename) {
  if (!filename.ends_with(".png")) { filename += ".png"; }
  auto data_copy = image.data();
  auto size_px_copy = atlas_size_px;

  std::thread th([filename, size_px_copy, data_copy]() {
    i32 status = stbi_write_png(filename.c_str(), size_px_copy, size_px_copy, 4, data_copy, 4 * size_px_copy);
    if (status == 1) {
      out::debug_info("Saved texture atlas to {}", filename);
    } else {
      out::debug_warning("Couldn't save texture atlas to {}", filename);
    }
  });
  th.detach();
}

std::optional<std::reference_wrapper<TextureAtlasData>> TextureAtlas::get_internal(std::string_view id, i32 max_depth) {
  if (max_depth <= 0) {
    if (fallback_texture.has_value()) {
      return get(fallback_texture.value());
    } else {
      return std::nullopt;
    }
  }

  if (auto it = textures.find(id); it != textures.end()) {
    return it->second;
  } else if (auto aliases_it = aliases.find(id); aliases_it != aliases.end()) {
    return get_internal(aliases_it->second, max_depth - 1);
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

void TextureAtlas::set_fallback_texture(std::string_view id) {
  if (!has_texture(id)) { return; }
  fallback_texture = id;
}

void TextureAtlas::regenerate_texture() {
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, atlas_size_px, atlas_size_px);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlas_size_px, atlas_size_px, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
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
  for (i32 square_x = 0; square_x < (atlas_size_px / grid_size_px); square_x += 1) {
    for (i32 square_y = 0; square_y < (atlas_size_px / grid_size_px); square_y += 1) {
      bool fail = false;
      for (i32 square_width = 0; square_width < std::ceil(width / (double)grid_size_px); square_width += 1) {
        for (i32 square_height = 0; square_height < std::ceil(height / (double)grid_size_px); square_height += 1) {
          size_t square_i = (atlas_size_px / grid_size_px) * (square_y + square_height) + (square_x + square_width);
          // size_t square_i = square_x + square_y * (atlas_size / square_size_px);
          if (square_x + square_width >= (atlas_size_px / grid_size_px) ||
              square_y + square_height >= (atlas_size_px / grid_size_px)) {
            fail = true;
            break;
          }
          if (occupied_grid_space[square_i]) {
            fail = true;
            break;
          }
        }
        if (fail) { break; }
      }

      if (!fail) { return std::make_pair(square_x * grid_size_px, square_y * grid_size_px); }
    }
  }
  out::debug_warning("find_space_for_texture, no space found for texture of size {}px by {}px", width, height);
  return std::nullopt;
}

void TextureAtlas::mark_space_for_texture(i32 x, i32 y, i32 width, i32 height, bool unmark) {
  i32 begin_x = x / grid_size_px;
  i32 begin_y = y / grid_size_px;
  i32 end_x = begin_x + std::ceil(width / (double)grid_size_px);
  i32 end_y = begin_y + std::ceil(height / (double)grid_size_px);

  for (i32 square_x = begin_x; square_x < end_x; square_x += 1) {
    for (i32 square_y = begin_y; square_y < end_y; square_y += 1) {
      if (!unmark) { ensure(!occupied_grid_space[(atlas_size_px / grid_size_px) * square_y + square_x]); }
      occupied_grid_space[(atlas_size_px / grid_size_px) * square_y + square_x] = !unmark;
    }
  }
}

vec2i TextureAtlas::paste_texture(stbi_uc* data, i32 width, i32 height) {
  auto [x, y] = find_space_for_texture(width + 2 * margin_px, height + 2 * margin_px).value_or(std::make_pair(0, 0));
  mark_space_for_texture(x + margin_px, y + margin_px, width + 2 * margin_px, height + 2 * margin_px);

  for (i32 oy = 0; oy < height; oy += 1) {
    for (i32 ox = 0; ox < width; ox += 1) {
      if ((x + ox) < atlas_size_px && (y + oy) < atlas_size_px) {
        for (i32 c = 0; c < 4; c += 1) {
          image[4 * ((y + oy) * atlas_size_px + (x + ox)) + c] = data[4 * (oy * width + ox) + c];
        }
      }
    }
  }
  dirty = true;
  return {x, y};
}
