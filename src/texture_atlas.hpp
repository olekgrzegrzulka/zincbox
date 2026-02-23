#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "../lib/stb_image.h"
#include "types.hpp"

struct TextureAtlasData {
  vec2f start;
  vec2f end;
  i32 width;
  i32 height;
};

class TextureAtlas {
public:
  TextureAtlas(i32 atlas_size_ = 512, i32 margin_px_ = 2, i32 min_texture_size_px_ = 4);

  bool has_texture(std::string id) const {
    return textures.contains(id);
  }

  bool add_texture(std::string id, std::string path);

  bool add_texture(std::string id, const std::vector<u8>& data, i32 width, i32 height);

  void save_to_file(std::string filename);

  std::optional<std::reference_wrapper<TextureAtlasData>> get(std::string id);

  void bind(u32 slot);

protected:
  std::optional<std::pair<i32, i32>> find_space_for_texture(i32 width, i32 height);

  void mark_space_for_texture(i32 x, i32 y, i32 width, i32 height);

  vec2i paste_texture(stbi_uc* data, i32 width, i32 height);

  float half_pixel() { return 1.0 / (atlas_size * 2.0); }

  void regenerate_texture();

  u32 texture = 0;
  u32 sampler = 0;
  bool dirty = true;
  std::vector<u8> image;
  std::unordered_map<std::string, TextureAtlasData> textures;

  std::vector<bool> free_16x16_squares;

  i32 atlas_size = 512;
  i32 margin_px = 2;
  i32 min_texture_size_px = 4;
};
