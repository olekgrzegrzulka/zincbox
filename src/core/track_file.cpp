#include "track_file.hpp"
#include <filesystem>
#include <string>
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>
#include <tfile.h>
#include <tfilestream.h>

namespace fs = std::filesystem;

zincbox::scanner::TrackFile::TrackFile(const fs::path& path) {
  file_path = path;
  TagLib::FileStream fstream(file_path.c_str(), true);
  TagLib::FileRef f(&fstream);

  if (f.isNull() || !f.file()) {
    error = true;
    return;
  }

  TagLib::PropertyMap properties = f.file()->properties();

  if (auto* tag = f.tag()) {
    metadata.title = tag->title().to8Bit(true);
    metadata.artist = tag->artist().to8Bit(true);
    metadata.album = tag->album().to8Bit(true);
    metadata.genre = tag->genre().to8Bit(true);
    if (tag->track() > 0) { metadata.track_number = tag->track(); }
    if (tag->year() > 0) { metadata.year = tag->year(); }
  }

  if (properties.contains("ALBUMARTIST")) {
    auto artistList = properties["ALBUMARTIST"];
    for (const auto& artist_ : artistList) {
      metadata.album_artist.emplace_back(artist_.to8Bit(true));
    }
  }
  if (properties.contains("TRACKTOTAL")) { metadata.track_total = properties["TRACKTOTAL"].front().toInt(); }

  if (auto* audio_props = f.audioProperties()) {
    metadata.duration_ms = audio_props->lengthInMilliseconds();
    metadata.bitrate_kb_s = audio_props->bitrate();
  }

  file_size = static_cast<u64>(fs::file_size(file_path));
  auto ftime = fs::last_write_time(file_path);
  last_modified = std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();
}
