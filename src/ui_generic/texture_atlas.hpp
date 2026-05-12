#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"
#include "lib/stb_image/stb_image.h"

struct TextureAtlasData {
    vec2f start;
    vec2f end;
    i32 width;
    i32 height;
};

class TextureAtlas {
  public:
    TextureAtlas(i32 atlas_size_ = 1024, i32 margin_px_ = 1, i32 grid_size_px_ = 4);

    bool has_texture(std::string id, i32 max_depth = 16) const {
      if (max_depth <= 0) { return false; }
      if (textures.contains(id)) { return true; }
      return (aliases.contains(id) && has_texture(aliases.at(id), max_depth - 1));
    }
    bool add_texture(std::string id, std::string path);
    bool add_texture(std::string id, const std::vector<u8>& data, i32 width, i32 height);
    bool add_texture(std::string id, const u8* data, i32 width, i32 height);
    void add_texture_alias(std::string id, std::string to);
    bool remove_texture(std::string id);

    void save_to_file(std::string filename);

    std::optional<std::reference_wrapper<TextureAtlasData>> get(std::string id) { return get_internal(id, 16); }

    void bind(u32 slot);

    void set_fallback_texture(std::string id);

  protected:
    std::optional<std::reference_wrapper<TextureAtlasData>> get_internal(std::string id, i32 max_depth = 16);
    std::optional<std::pair<i32, i32>> find_space_for_texture(i32 width, i32 height);

    void mark_space_for_texture(i32 x, i32 y, i32 width, i32 height, bool unmark = false);

    vec2i paste_texture(stbi_uc* data, i32 width, i32 height);

    float half_pixel() { return 1.0 / (atlas_size_px * 2.0); }

    void regenerate_texture();

    u32 texture = 0;
    u32 sampler = 0;
    bool dirty = true;
    std::vector<u8> image;
    std::unordered_map<std::string, TextureAtlasData> textures;
    std::unordered_map<std::string, std::string> aliases;
    std::optional<std::string> fallback_texture;

    std::vector<bool> occupied_grid_space;

    i32 atlas_size_px = 1024;
    i32 margin_px = 1;
    i32 grid_size_px = 4;
};
