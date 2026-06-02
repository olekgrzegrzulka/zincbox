#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "common/types.hpp"
#include "musicdb/track.hpp"

namespace db {
  struct Playlist;
}

namespace TagLib {
  class FileRef;
}

namespace io {
  namespace fs = std::filesystem;

  std::vector<u8> resize_album_art_to_64x64(const std::vector<u8>& raw_data);
  std::vector<u8> fetch_album_art(const TagLib::FileRef&);
  bool is_cover_file(fs::path path);
  bool is_music_file(fs::path path);
  bool add_cover_file(db::Playlist& playlist, fs::path path);

  fs::path get_user_data_path();
  fs::path get_themes_path();
  fs::path get_db_path();

  class TrackFile final {
    public:
      TrackFile(fs::path path);

      TrackFile(TrackFile&& other);
      TrackFile& operator=(TrackFile&& other);
      TrackFile(const TrackFile&) = delete;
      TrackFile& operator=(const TrackFile&) = delete;

      bool is_valid() const { return track.has_value(); }
      std::optional<db::Track> get_track() const { return track; }
      std::vector<u8> get_album_art();
      std::u32string get_album_name() const { return album_name; }
      std::u32string get_album_artist() const { return album_artist; }

    private:
      std::optional<db::Track> track;
      std::vector<u8> album_art;
      std::u32string album_name;
      std::u32string album_artist;
  };

} // namespace io
