#pragma once
#include <optional>
#include <string>
#include "core/musicdb/types.hpp"

namespace zincgui {
  class TextureAtlas;
} // namespace zincgui

namespace zincbox::ui::cover_art {
  zincgui::TextureAtlas& texture_atlas();
  void init();
  bool update();
  void bind();
  std::optional<std::string> get_texture_id(db::track_id_t track_id);
} // namespace zincbox::ui::cover_art
