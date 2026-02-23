#pragma once
#include "types.hpp"

class PanelAlbums;
class PanelTracks;

namespace bridge {
  void init(PanelTracks*, PanelAlbums*);

  void on_album_clicked(i32 id);
}; // namespace bridge
