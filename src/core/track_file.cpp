#include "track_file.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>
#include <tfile.h>
#include <tfilestream.h>
#include "common/debug.hpp"
#include "core/io.hpp"

#define XXH_INLINE_ALL
#include <lib/xxHash/xxhash.h>

namespace fs = std::filesystem;
static std::string hash_taglib_file(TagLib::File* file);

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

  hash = hash_taglib_file(f.file());
}

class TrackHasher {
  public:
    TrackHasher() {
      state_ = XXH3_createState();
      ensure(state_);
    }

    ~TrackHasher() {
      if (state_) { XXH3_freeState(state_); }
    }

    TrackHasher(const TrackHasher&) = delete;
    TrackHasher& operator=(const TrackHasher&) = delete;

    std::string hash_file(TagLib::File* file) {
      if (!file || !file->isValid()) { return ""; }

      XXH3_128bits_reset(state_);

      file->seek(0, TagLib::File::Beginning);

      constexpr unsigned long buffer_size = 128 * 1024;
      TagLib::ByteVector buffer;

      while (true) {
        buffer = file->readBlock(buffer_size);
        if (buffer.isEmpty()) { break; }
        XXH3_128bits_update(state_, buffer.data(), buffer.size());
      }

      XXH128_hash_t hash = XXH3_128bits_digest(state_);

      return to_hex(hash);
    }

  private:
    XXH3_state_t* state_{nullptr};

    std::string to_hex(const XXH128_hash_t& hash) {
      static const char hex_digits[] = "0123456789abcdef";
      std::string result(32, '0');

      auto write64 = [&](uint64_t val, char* dest) {
        for (int i = 15; i >= 0; --i) {
          dest[i] = hex_digits[val & 0x0F];
          val >>= 4;
        }
      };

      write64(hash.high64, &result[0]);
      write64(hash.low64, &result[16]);

      return result;
    }
};

static std::string hash_taglib_file(TagLib::File* file) {
  thread_local TrackHasher hasher;
  return hasher.hash_file(file);
}
