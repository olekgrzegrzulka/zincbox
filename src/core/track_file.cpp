#include "track_file.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/flac/flacfile.h>
#include <taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tfile.h>
#include <taglib/toolkit/tfilestream.h>
#include <taglib/toolkit/tpropertymap.h>

namespace {

  std::string trim_cr(std::string_view sv) {
    if (!sv.empty() && sv.back() == '\r') { sv.remove_suffix(1); }
    return std::string(sv);
  }

  std::vector<zincbox::LyricLine> parse_lrc(std::string_view raw) {
    std::vector<zincbox::LyricLine> synced;
    i32 offset_ms = 0;
    size_t pos = 0;

    while (pos < raw.size()) {
      size_t line_end = raw.find('\n', pos);
      if (line_end == std::string_view::npos) { line_end = raw.size(); }

      std::string_view line = raw.substr(pos, line_end - pos);
      if (!line.empty() && line.back() == '\r') { line.remove_suffix(1); }
      pos = line_end + 1;

      if (line.empty() || line.front() != '[') { continue; }

      if (line.starts_with("[offset:")) {
        size_t close = line.find(']');
        if (close != std::string_view::npos) {
          std::string_view val = line.substr(8, close - 8);
          if (!val.empty() && val.front() == '+') { val.remove_prefix(1); }
          std::from_chars(val.data(), val.data() + val.size(), offset_ms);
        }
        continue;
      }

      std::vector<i32> line_timestamps;
      size_t cursor = 0;

      while (cursor < line.size() && line[cursor] == '[') {
        size_t close = line.find(']', cursor);
        if (close == std::string_view::npos) { break; }

        std::string_view tag_content = line.substr(cursor + 1, close - (cursor + 1));
        size_t colon = tag_content.find(':');
        if (colon == std::string_view::npos) { break; }

        i32 minutes = 0;
        i32 seconds = 0;
        auto min_res = std::from_chars(tag_content.data(), tag_content.data() + colon, minutes);
        if (min_res.ec != std::errc{} || min_res.ptr != tag_content.data() + colon) { break; }

        std::string_view sec_part = tag_content.substr(colon + 1);
        size_t sep = sec_part.find_first_of(".:");
        std::string_view sec_whole = (sep != std::string_view::npos) ? sec_part.substr(0, sep) : sec_part;

        auto sec_res = std::from_chars(sec_whole.data(), sec_whole.data() + sec_whole.size(), seconds);
        if (sec_res.ec != std::errc{} || sec_res.ptr != sec_whole.data() + sec_whole.size()) { break; }

        i32 ms = (minutes * 60 + seconds) * 1000;

        if (sep != std::string_view::npos) {
          std::string_view frac_part = sec_part.substr(sep + 1);
          i32 frac = 0;
          auto frac_res = std::from_chars(frac_part.data(), frac_part.data() + frac_part.size(), frac);
          if (frac_res.ec == std::errc{}) {
            if (frac_part.size() == 1) {
              ms += frac * 100;
            } else if (frac_part.size() == 2) {
              ms += frac * 10;
            } else if (frac_part.size() >= 3) {
              while (frac >= 1000) {
                frac /= 10;
              }
              ms += frac;
            }
          }
        }

        line_timestamps.push_back(ms);
        cursor = close + 1;
      }

      if (!line_timestamps.empty()) {
        std::string_view lyric_text = line.substr(cursor);
        if (!lyric_text.empty() && lyric_text.front() == ' ') { lyric_text.remove_prefix(1); }
        std::string clean_text = trim_cr(lyric_text);
        for (i32 ts : line_timestamps) {
          i32 adjusted = std::max<i32>(0, ts - offset_ms);
          synced.push_back({adjusted, clean_text});
        }
      }
    }

    std::stable_sort(synced.begin(), synced.end(), [](const zincbox::LyricLine& a, const zincbox::LyricLine& b) {
      return a.timestamp_ms < b.timestamp_ms;
    });

    return synced;
  }

  void assign_text_or_lrc(zincbox::Lyrics& out_lyrics, std::string_view raw_text) {
    if (raw_text.empty()) { return; }
    auto parsed = parse_lrc(raw_text);
    if (!parsed.empty()) {
      if (out_lyrics.synced.empty()) { out_lyrics.synced = std::move(parsed); }
    } else if (out_lyrics.unsynced.empty()) {
      out_lyrics.unsynced = raw_text;
    }
  }

  void extract_id3v2_lyrics(TagLib::ID3v2::Tag* id3v2, zincbox::Lyrics& out_lyrics) {
    if (id3v2 == nullptr) { return; }

    const auto& sylt_frames = id3v2->frameListMap()["SYLT"];
    for (auto* frame : sylt_frames) {
      auto* sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame*>(frame);
      if (sylt == nullptr || sylt->synchedText().isEmpty()) { continue; }

      bool is_ms = (sylt->timestampFormat() == TagLib::ID3v2::SynchronizedLyricsFrame::AbsoluteMilliseconds);
      for (const auto& item : sylt->synchedText()) {
        i32 ms = is_ms ? static_cast<i32>(item.time) : static_cast<i32>(item.time * 1000 / 38);
        out_lyrics.synced.push_back({ms, item.text.to8Bit(true)});
      }
      if (!out_lyrics.synced.empty()) { break; }
    }

    const auto& uslt_frames = id3v2->frameListMap()["USLT"];
    for (auto* frame : uslt_frames) {
      auto* uslt = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frame);
      if (uslt != nullptr && !uslt->text().isEmpty()) {
        assign_text_or_lrc(out_lyrics, uslt->text().to8Bit(true));
        break;
      }
    }
  }

  void extract_lyrics(TagLib::File* file, const TagLib::PropertyMap& properties,
                      const std::filesystem::path& track_path, zincbox::Lyrics& out_lyrics) {
    if (auto* mpeg_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
      if (mpeg_file->hasID3v2Tag()) { extract_id3v2_lyrics(mpeg_file->ID3v2Tag(false), out_lyrics); }
    } else if (auto* flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
      if (flac_file->hasID3v2Tag()) { extract_id3v2_lyrics(flac_file->ID3v2Tag(false), out_lyrics); }
    }

    for (const auto& [key, val_list] : properties) {
      if (val_list.isEmpty()) { continue; }
      std::string key_upper = key.upper().to8Bit(true);
      if (key_upper == "LYRICS" || key_upper == "UNSYNCEDLYRICS" || key_upper == "SYNCEDLYRICS" ||
          key_upper == "\xA9LYR" || key_upper.starts_with("LYRICS:") || key_upper.starts_with("UNSYNCEDLYRICS:")) {
        assign_text_or_lrc(out_lyrics, val_list.front().to8Bit(true));
      }
    }

    if (out_lyrics.synced.empty()) {
      std::error_code ec;
      std::filesystem::path lrc_path = track_path;
      lrc_path.replace_extension(".lrc");
      if (std::filesystem::is_regular_file(lrc_path, ec)) {
        std::ifstream lrc_stream(lrc_path, std::ios::in | std::ios::binary);
        if (lrc_stream) {
          std::ostringstream ss;
          ss << lrc_stream.rdbuf();
          assign_text_or_lrc(out_lyrics, ss.str());
        }
      }
    }
  }

} // namespace

zincbox::scanner::TrackFile::TrackFile(const fs::path& path) {
  file_path = path;
  TagLib::FileStream fstream(file_path.c_str(), true);
  TagLib::FileRef f(&fstream);

  if (f.isNull() || (f.file() == nullptr)) {
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
    auto artist_list = properties["ALBUMARTIST"];
    for (const auto& artist_ : artist_list) {
      metadata.album_artist.emplace_back(artist_.to8Bit(true));
    }
  }
  if (properties.contains("TRACKTOTAL")) { metadata.track_total = properties["TRACKTOTAL"].front().toInt(); }

  extract_lyrics(f.file(), properties, file_path, metadata.lyrics);

  if (auto* audio_props = f.audioProperties()) {
    metadata.duration_ms = audio_props->lengthInMilliseconds();
    metadata.bitrate_kb_s = audio_props->bitrate();
  }

  file_size = static_cast<u64>(fs::file_size(file_path));
  auto ftime = fs::last_write_time(file_path);
  last_modified = std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();
}
