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
  std::optional<std::string> get_texture_id(db::track_id_t, bool high_res = false);
  std::optional<std::string> get_texture_id_of_playlist(db::playlist_id_t);
  std::optional<std::string> get_texture_id_of_collection(db::collection_id_t);
} // namespace zincbox::ui::cover_art
