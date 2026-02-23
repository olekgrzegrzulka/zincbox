#include "bridge.hpp"
#include "debug.hpp"
#include "panel_albums.hpp"
#include "panel_tracks.hpp"

PanelTracks* panel_tracks;
PanelAlbums* panel_albums;

void bridge::init(PanelTracks* panel_tracks_, PanelAlbums* panel_albums_) {
  panel_tracks = panel_tracks_;
  panel_albums = panel_albums_;
}

void bridge::on_album_clicked(i32 id) {
  ensure(panel_tracks);
  panel_tracks->on_album_clicked(id);
}
