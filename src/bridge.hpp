#pragma once
#include "types.hpp"

class PanelAlbums;
class PanelTracks;

namespace musicdb {
  struct Album;
  struct Track;
} // namespace musicdb

namespace bridge {
  void init(PanelTracks*, PanelAlbums*);

  void on_album_clicked(const musicdb::Album*);
}; // namespace bridge
