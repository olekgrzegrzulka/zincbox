#pragma once
#include <string>
#include "common/types.hpp"

class Texture {
  public:
    Texture(const std::string& file_name);

    void bind(u32 slot) const;

  private:
    static u32 load_texture(const std::string& file_path);

    static u32 create_sampler();

  private:
    u32 texture = 0;
    u32 sampler = 0;
};
