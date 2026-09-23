#pragma once
#include <filesystem>
#include <string>
#include "core/musicdb/track_metadata.hpp"

namespace zincbox {
  namespace scanner {
    namespace fs = std::filesystem;

    struct TrackFile {
        bool error{};
        zincbox::TrackMetadata metadata;
        fs::path file_path;
        fs::path file_name_without_extension() const { return file_path.stem(); }
        fs::path file_extension() const { return file_path.extension(); }
        fs::path parent_directory_name() const { return file_path.parent_path().stem(); }
        std::string hash{};

        TrackFile(const fs::path& path);

        TrackFile() = default;
        TrackFile(const TrackFile&) = delete;
        TrackFile& operator=(const TrackFile&) = delete;
        TrackFile(TrackFile&&) noexcept = default;
        TrackFile& operator=(TrackFile&&) noexcept = default;
    };

  } // namespace scanner
} // namespace zincbox
