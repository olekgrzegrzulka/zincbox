#include "texture.hpp"
#include <filesystem>
#include <string>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "lib/stb_image/stb_image.h"
#include "opengl_includes.hpp"

zincgui::Texture::Texture(const std::filesystem::path& file_name) {
  std::string file_path = "./assets/" / file_name;
  texture = Texture::load_texture(file_path);
  sampler = Texture::create_sampler();
}

void zincgui::Texture::bind(u32 slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindSampler(slot, sampler);
}

u32 zincgui::Texture::load_texture(const std::filesystem::path& path) {
  int width, height, channels;

#if defined(_WIN32)
  FILE* f = _wfopen(path.c_str(), L"rb");
#else
  FILE* f = fopen(path.c_str(), "rb");
#endif
  if (!f) {
    out::critical("load_texture({}): {}", path_to_utf8(path), "fopen returned false");
    exit(1);
  }

  stbi_uc* data = stbi_load_from_file(f, &width, &height, &channels, STBI_rgb_alpha);
  fclose(f);

  if (!data) {
    out::critical("load_texture({}): {}", path_to_utf8(path), stbi_failure_reason());
    exit(1);
  }

  GLuint texture_ = 0;

  glGenTextures(1, &texture_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_);

  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);

  return texture_;
}

u32 zincgui::Texture::create_sampler() {
  u32 sampler_ = 0;
  glCreateSamplers(1, &sampler_);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return sampler_;
}
