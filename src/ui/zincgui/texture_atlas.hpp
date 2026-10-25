#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "common/types.hpp"
#include "lib/stb_image/stb_image.h"
#include "ui/zincgui/default_settings.hpp"

namespace zincgui {

  struct TextureAtlasData {
      vec2f start;
      vec2f end;
      i32 width;
      i32 height;
  };

  class TextureAtlas {
    public:
      TextureAtlas(i32 atlas_size_ = default_settings::texture_atlas_size,
                   i32 margin_px_ = default_settings::texture_atlas_margin,
                   i32 grid_size_px_ = default_settings::texture_atlas_grid_size);

      i32 texture_count() const { return textures.size(); }

      bool has_texture(std::string_view id, i32 max_depth = default_settings::texture_atlas_max_alias_depth) const {
        if (max_depth <= 0) { return false; }
        if (textures.contains(id)) { return true; }
        return (aliases.contains(id) && has_texture(aliases.at(std::string(id)), max_depth - 1));
      }

      bool add_texture(std::string_view id, const std::filesystem::path&);
      bool add_texture(std::string_view id, const std::vector<u8>& data, i32 width, i32 height);
      bool add_texture(std::string_view id, const u8* data, i32 width, i32 height);
      bool add_texture_row(std::span<const std::string> ids, const std::filesystem::path&);
      bool add_texture_row(std::span<const std::string> ids, const u8* data, i32 width, i32 height);
      void add_texture_alias(std::string id, std::string to);
      bool remove_texture(std::string_view id);
      vec2i get_texture_size(std::string_view id);
      bool can_fit_texture(i32 w, i32 h);

      void save_to_file(std::string filename);

      std::optional<std::reference_wrapper<TextureAtlasData>> get(std::string_view id) {
        return get_internal(id, default_settings::texture_atlas_max_alias_depth);
      }

      void bind(u32 slot);

      void set_fallback_texture(std::string_view id);

      i32 size() const { return atlas_size_px; }

      void set_filter(bool f) {
        if (filter != f) {
          filter = f;
          regenerate_texture();
        }
      }

    protected:
      std::optional<std::reference_wrapper<TextureAtlasData>>
      get_internal(std::string_view id, i32 max_depth = default_settings::texture_atlas_max_alias_depth);
      std::optional<std::pair<i32, i32>> find_space_for_texture(i32 width, i32 height);

      void mark_space_for_texture(i32 x, i32 y, i32 width, i32 height, bool unmark = false);

      std::optional<vec2i> paste_texture(stbi_uc* data, i32 width, i32 height);

      void regenerate_texture();

      u32 texture = 0;
      u32 sampler = 0;
      bool dirty = true;
      std::vector<u8> image;
      struct StringHash {
          using is_transparent = void;
          size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
      };
      std::unordered_map<std::string, TextureAtlasData, StringHash, std::equal_to<>> textures;
      std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> aliases;
      std::optional<std::string> fallback_texture;

      std::vector<bool> occupied_grid_space;

      i32 atlas_size_px = default_settings::texture_atlas_size;
      i32 margin_px = 1;
      i32 grid_size_px = default_settings::texture_atlas_grid_size;
      bool filter = false;
  };

} // namespace zincgui
