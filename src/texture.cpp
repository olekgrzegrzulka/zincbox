#include "texture.hpp"
#include <string>
#include "../lib/stb_image.h"
#include "debug.hpp"
#include "opengl_includes.hpp"
#include "types.hpp"

Texture::Texture(std::string file_name) {
  std::string file_path = "./assets/" + file_name;
  texture = Texture::load_texture(file_path);
  sampler = Texture::create_sampler();
}

void Texture::bind(u32 slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindSampler(slot, sampler);
}

u32 Texture::load_texture(std::string file_path) {
  int width, height, channels;
  stbi_uc* data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!data) {
    debug_error("failed to load texture" + file_path);
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

u32 Texture::create_sampler() {
  u32 sampler_ = 0;
  glCreateSamplers(1, &sampler_);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return sampler_;
}
