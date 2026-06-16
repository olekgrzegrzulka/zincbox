#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "common/types.hpp"
#include "core/musicdb/track.hpp"
#include "lib/stb_image/stb_image.h"

namespace TagLib {
  class FileRef;
}

class TrackFile final {
  public:
    TrackFile(const std::filesystem::path& path, bool fetch_album_art);
    bool fetch_album_art(TagLib::FileRef* ref = nullptr);

    TrackFile(TrackFile&& other) noexcept;
    TrackFile& operator=(TrackFile&& other) noexcept;
    TrackFile(const TrackFile&) = delete;
    TrackFile& operator=(const TrackFile&) = delete;

  public:
    static std::optional<std::filesystem::path> save_album_art(const u8* data, size_t size);
    static std::optional<std::filesystem::path> save_album_art(stbi_uc* img, i32 width, i32 height, i32 channels);
    static std::vector<u8> resize_album_art_to_64x64(const u8* data, size_t size);
    static std::vector<u8> resize_album_art_to_64x64(stbi_uc* img, i32 width, i32 height, i32 channels);

  public:
    std::filesystem::path path;
    std::optional<db::Track> track;
    std::vector<u8> album_art_64x64;
    std::optional<std::filesystem::path> album_art_path;
    std::u32string album_name;
    std::u32string album_artist;
};
