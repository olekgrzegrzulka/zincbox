#pragma once
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "core/track_file.hpp"

namespace zincbox::scanner {

  struct ScanProgress {
      i32 directories_scanned = 0;
      i32 files_scanned = 0;
      auto operator<=>(const ScanProgress&) const = default;
  };

  struct ScanResult {
      struct DirContent {
          std::vector<std::filesystem::path> image_files;
          std::vector<TrackFile> track_files;
      };

      std::vector<std::filesystem::path> errors;
      std::unordered_map<std::filesystem::path, DirContent> files;
  };

  struct ImportSummary {
      db::collection_id_t collection_id{};
      std::vector<db::track_id_t> skipped_tracks;
      std::vector<db::track_id_t> added_tracks;
      std::vector<db::track_id_t> modified_tracks;
      std::vector<db::track_id_t> not_found_tracks;
      std::vector<std::filesystem::path> errors;

      ImportSummary() = default;
      ImportSummary(const ImportSummary&) = delete;
      ImportSummary& operator=(const ImportSummary&) = delete;
      ImportSummary(ImportSummary&&) noexcept = default;
      ImportSummary& operator=(ImportSummary&&) noexcept = default;
  };

  void scan_collection(db::collection_id_t);
  std::optional<ScanProgress> get_progress();
  std::optional<ImportSummary> import();
} // namespace zincbox::scanner
