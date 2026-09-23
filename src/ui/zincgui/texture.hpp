#pragma once
#include <filesystem>
#include "common/types.hpp"

namespace zincgui {

  class Texture {
    public:
      Texture(const std::filesystem::path&);

      void bind(u32 slot) const;

    private:
      static u32 load_texture(const std::filesystem::path&);

      static u32 create_sampler();

    private:
      u32 texture = 0;
      u32 sampler = 0;
  };

} // namespace zincgui
