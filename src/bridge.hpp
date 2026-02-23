#pragma once
#include "types.hpp"

class PanelAlbums;
class PanelTracks;

namespace musicdb {
  struct Album;
  struct Track;

  using album_id_t = size_t;
  using track_id_t = size_t;
} // namespace musicdb

namespace bridge {
  void init(PanelTracks*, PanelAlbums*);

  void on_album_clicked(musicdb::album_id_t);
}; // namespace bridge
