#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "common/types.hpp"
#include "common/worker.hpp"
#include "core/io.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/scanner.hpp"
#include "core/track_file.hpp"

namespace sc = zincbox::scanner;
namespace fs = std::filesystem;

void task_scan_directory(fs::path scanned_directory, JobContext<sc::ScanProgress, sc::ScanResult> ctx);
using ScannerWorker = Worker<task_scan_directory, sc::ScanProgress, sc::ScanResult>;

static ScannerWorker s_worker;
static std::unordered_map<fs::path, std::pair<ScannerWorker::job_id_t, db::collection_id_t>> s_scanned_paths;
static std::vector<std::pair<std::unique_ptr<zincbox::scanner::ScanResult>, db::collection_id_t>> s_scan_results;

void task_scan_directory(fs::path root, JobContext<sc::ScanProgress, sc::ScanResult> ctx) {
  if (!fs::is_directory(root)) {
    ctx.set_result(sc::ScanResult{});
    return;
  }

  ctx.set_progress(sc::ScanProgress{.files_scanned = 0});

  sc::ScanProgress prog;
  sc::ScanResult res;

  for (auto& file : fs::recursive_directory_iterator(root)) {
    if (!file.path().has_root_directory()) { continue; }
    auto parent_directory = file.path().root_directory();
    if (file.is_regular_file()) {
      if (io::is_cover_file(file)) {
        res.files[parent_directory].image_files.emplace_back(file.path());
        prog.files_scanned += 1;
      } else if (io::is_music_file(file)) {
        auto track_file = sc::TrackFile(file.path());
        if (track_file.error) {
          res.errors.emplace_back(track_file.file_path);
        } else {
          res.files[parent_directory].track_files.emplace_back(std::move(track_file));
        }
        prog.files_scanned += 1;
      }
    } else if (file.is_directory()) {
      prog.directories_scanned += 1;
    }

    ctx.set_progress(prog);
  }

  ctx.set_result(std::move(res));
  return;
}

void sc::scan_directory(fs::path dir, db::collection_id_t collection_id) {
  if (collection_id == 0) { return; }
  if (s_scanned_paths.contains(dir)) { return; }
  s_scanned_paths[dir] = {s_worker.run(dir), collection_id};
}

i32 compute_similiarity_index(const db::Track& a, const zincbox::scanner::TrackFile& b) {
  return (i32)(a.metadata.album == b.metadata.album) + (i32)(a.metadata.album_artist == b.metadata.album_artist) +
         (i32)(a.metadata.genre == b.metadata.genre) + (i32)(a.metadata.track_number == b.metadata.track_number) +
         (i32)(a.metadata.track_total == b.metadata.track_total) + (i32)(a.metadata.year == b.metadata.year) +
         (i32)(a.metadata.duration_ms == b.metadata.duration_ms) +
         (i32)(a.metadata.bitrate_kb_s == b.metadata.bitrate_kb_s) +
         (i32)(a.parent_directory_name() == b.parent_directory_name());
}

std::optional<db::track_id_t> match_track(db::collection_id_t c_id, const sc::TrackFile& track_file) {
  auto track_at_same_path = db::track_by_path(track_file.file_path);

  if (track_at_same_path) {
    bool same_collection = db::collection_of_track(*track_at_same_path) == c_id;
    bool not_found_yet = db::track_by_id(*track_at_same_path)->get().get_flag(db::NOT_FOUND_DURING_RESCAN);
    if (same_collection && not_found_yet) { return track_at_same_path.value(); }
  }

  std::unordered_set<db::track_id_t> track_with_same_hash = db::track_by_hash(track_file.hash);
  for (auto track : track_with_same_hash) {
    bool not_found_yet = db::track_by_id(track)->get().get_flag(db::NOT_FOUND_DURING_RESCAN);
    std::optional<std::pair<db::track_id_t, i32>> best_track;

    if (db::collection_of_track(track) == c_id && not_found_yet) {
      i32 score = compute_similiarity_index(db::track_by_id(track)->get(), track_file);
      score += (i32)(track_file.metadata.title == db::track_by_id(track)->get().metadata.title);
      score += (i32)(track_file.metadata.artist == db::track_by_id(track)->get().metadata.artist);
      if (!best_track || best_track->second < score) { best_track = {track, score}; }
    }

    if (best_track) { return best_track->first; }
  }

  auto tracks_with_same_title_and_artist =
    db::track_by_artist_title(track_file.metadata.artist, track_file.metadata.title);
  auto tracks_with_same_file_name = db::track_by_file_name(track_file.file_name_without_extension());
  std::optional<std::pair<db::track_id_t, i32>> best_track;
  for (db::track_id_t id : tracks_with_same_title_and_artist) {
    if (db::collection_of_track(id) != c_id) { continue; }
    if (!db::track_by_id(id)->get().get_flag(db::NOT_FOUND_DURING_RESCAN)) { continue; }
    i32 score = compute_similiarity_index(db::track_by_id(id)->get(), track_file);
    if (!best_track || best_track->second < score) { best_track = {id, score}; }
  }
  for (db::track_id_t id : tracks_with_same_file_name) {
    if (db::collection_of_track(id) != c_id) { continue; }
    if (!db::track_by_id(id)->get().get_flag(db::NOT_FOUND_DURING_RESCAN)) { continue; }
    i32 score = compute_similiarity_index(db::track_by_id(id)->get(), track_file);
    if (!best_track || best_track->second < score) { best_track = {id, score}; }
  }
  if (best_track && best_track->second > 0) {
    return best_track->first;
  } else {
    return std::nullopt;
  }
}

std::optional<sc::ScanProgress> sc::get_progress() {
  sc::ScanProgress total_progress{};
  std::vector<fs::path> to_erase;
  for (const auto& [path, s] : s_scanned_paths) {
    auto [job_id, collection_id] = s;
    auto job_status = s_worker.get(job_id);
    auto* progress = job_status.get_progress();
    total_progress.directories_scanned += progress ? progress->directories_scanned : 0;
    total_progress.files_scanned += progress ? progress->files_scanned : 0;

    if (job_status.is_result()) {
      s_scan_results.emplace_back(job_status.result(), collection_id);
      to_erase.emplace_back(path);
    }
  }

  for (auto& path : to_erase) {
    s_scanned_paths.erase(path);
  }

  if (total_progress == sc::ScanProgress{0, 0}) {
    return std::nullopt;
  } else {
    return total_progress;
  }
}

// FIXME: restore removed tracks
[[nodiscard]] std::optional<sc::ImportSummary> sc::import() {
  if (s_scan_results.empty()) { return std::nullopt; }
  std::unique_ptr<zincbox::scanner::ScanResult> scan_result = std::move(s_scan_results.back().first);
  db::collection_id_t collection_id = s_scan_results.back().second;
  s_scan_results.pop_back();
  if (!scan_result) { return std::nullopt; }

  // Mark all tracks in collection as not found
  const auto& collection = db::collection_by_id(collection_id)->get();
  for (db::playlist_id_t playlist_id : collection.playlist_ids()) {
    const auto& playlist = db::playlist_by_id(playlist_id)->get();
    for (db::track_id_t track_id : playlist.track_ids) {
      db::set_track_flag(track_id, db::NOT_FOUND_DURING_RESCAN, true);
    }
  }

  sc::ImportSummary import_summary;
  import_summary.collection_id = collection_id;

  for (auto&& [dir, dir_content] : scan_result.get()->files) {

    for (auto& track_file : dir_content.track_files) {
      if (auto track_id = match_track(collection_id, track_file); track_id) {
        auto& track = db::track_by_id(*track_id)->get();
        bool same_metadata = track.metadata == track_file.metadata;
        bool same_file_path = track.file_path == track_file.file_path;
        bool same_hash = track.hash == track_file.hash;
        if (same_metadata && same_file_path && same_hash) {
          db::set_track_flag(*track_id, db::NOT_FOUND_DURING_RESCAN, false);
          import_summary.skipped_tracks.emplace_back(*track_id);
        } else {
          db::set_track_flag(*track_id, db::NOT_FOUND_DURING_RESCAN, false);
          import_summary.modified_tracks.emplace_back(*track_id);
          db::set_track_hash(*track_id, track_file.hash);
          db::set_track_file_path(*track_id, track_file.file_path);
          db::set_track_metadata(*track_id, std::move(track_file.metadata));
        }
      } else {
        db::Track track;
        track.metadata = std::move(track_file.metadata);
        track.hash = track_file.hash;
        track.file_path = track_file.file_path;
        auto track_id_new = db::add_orphaned_track(collection_id, std::move(track));
        import_summary.added_tracks.emplace_back(track_id_new);
      }
    }
  }

  for (db::playlist_id_t playlist_id : collection.playlist_ids()) {
    const auto& playlist = db::playlist_by_id(playlist_id)->get();
    for (db::track_id_t track_id : playlist.track_ids) {
      if (db::track_by_id(track_id)->get().get_flag(db::NOT_FOUND_DURING_RESCAN)) {
        import_summary.not_found_tracks.emplace_back(track_id);
        db::mark_track_as_tombstone(track_id);
      }
    }
  }

  db::assign_orphaned_tracks();

  for (auto&& [dir, dir_content] : scan_result.get()->files) {

    for ([[maybe_unused]] auto& image_file : dir_content.image_files) {
      // update covers
    }

    // update covers
  }

  import_summary.errors = std::move(scan_result->errors);
  return import_summary;
}
