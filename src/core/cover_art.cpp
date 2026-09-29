
#include "cover_art.hpp"
//
#include <taglib/fileref.h>
//
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>
#include <taglib/attachedpictureframe.h>
#include <taglib/audioproperties.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/toolkit/tpropertymap.h>
#include <tfile.h>
#include <tfilestream.h>
#include "common/serialize.hpp"
#include "common/utf.hpp"
#include "common/worker.hpp"
#include "core/io.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "lib/stb_image/stb_image.h"
#include "lib/stb_image/stb_image_resize2.h"
#include "lib/stb_image/stb_image_write.h"
#define XXH_INLINE_ALL
#include "lib/xxHash/xxhash.h"

struct WorkerProgress {};

struct WorkerResult {
    db::track_id_t track_id;
    std::filesystem::path filename;
};

static void worker_fn(db::track_id_t, std::filesystem::path, JobContext<WorkerProgress, WorkerResult>);
static Worker<worker_fn, WorkerProgress, WorkerResult> worker;
static std::vector<decltype(worker)::job_id_t> active_jobs;

static std::vector<std::vector<std::filesystem::path>> filenames;
static std::unordered_set<db::track_id_t> track_ids_in_progress;

static void worker_fn(db::track_id_t track_id, std::filesystem::path path,
                      JobContext<WorkerProgress, WorkerResult> ctx) {
  TagLib::FileRef f(path.c_str());
  if (f.isNull()) {
    ctx.set_result({track_id, ""});
    return;
  }

  TagLib::ByteVector image_data;

  if (auto mpegFile = dynamic_cast<TagLib::MPEG::File*>(f.file())) {
    if (mpegFile->ID3v2Tag()) {
      auto frames = mpegFile->ID3v2Tag()->frameListMap()["APIC"];
      if (!frames.isEmpty()) {
        auto frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
        if (frame) { image_data = frame->picture(); }
      }
    }
  } else if (auto flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file())) {
    auto pictures = flacFile->pictureList();
    if (!pictures.isEmpty()) { image_data = pictures.front()->data(); }
  }

  std::vector<char> file_buffer;
  const char* final_data_ptr = image_data.data();
  size_t final_data_size = image_data.size();

  if (image_data.isEmpty()) {
    std::filesystem::path parent_dir = path.parent_path();
    std::filesystem::path found_image_path;

    if (std::filesystem::exists(parent_dir) && std::filesystem::is_directory(parent_dir)) {
      for (const auto& entry : std::filesystem::directory_iterator(parent_dir)) {
        if (entry.is_regular_file()) {
          std::string ext = entry.path().extension().string();
          for (char& c : ext) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
          }
          if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") { found_image_path = entry.path(); }
        }
      }
    }

    if (!found_image_path.empty()) {
      std::ifstream file(found_image_path, std::ios::binary | std::ios::ate);
      if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        file_buffer.resize(static_cast<size_t>(size));
        if (file.read(file_buffer.data(), size)) {
          final_data_ptr = file_buffer.data();
          final_data_size = file_buffer.size();
        }
      }
    }
  }

  if (final_data_size == 0) {
    ctx.set_result({track_id, ""});
    return;
  }

  XXH64_hash_t hash = XXH64(final_data_ptr, final_data_size, 0);
  std::string out_name = std::format("{:016x}.jpg", hash);

  std::filesystem::path full_dir = io::get_cover_cache_path() / "full";
  std::filesystem::path dir_256 = io::get_cover_cache_path() / "256";
  std::filesystem::path dir_64 = io::get_cover_cache_path() / "64";

  std::filesystem::create_directories(full_dir);
  std::filesystem::create_directories(dir_256);
  std::filesystem::create_directories(dir_64);

  if (std::filesystem::exists(full_dir / out_name) && std::filesystem::exists(dir_256 / out_name) &&
      std::filesystem::exists(dir_64 / out_name)) {
    ctx.set_result({track_id, out_name});
    return;
  }

  i32 width = 0;
  i32 height = 0;
  i32 channels = 0;
  unsigned char* data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(final_data_ptr),
                                              static_cast<int>(final_data_size), &width, &height, &channels, 4);

  if (!data) {
    ctx.set_result({track_id, ""});
    return;
  }

  stbi_write_jpg((full_dir / out_name).string().c_str(), width, height, 4, data, 90);

  unsigned char* data_256 = new unsigned char[1ll * 256 * 256 * 4];
  stbir_resize_uint8_linear(data, width, height, 0, data_256, 256, 256, 0, (stbir_pixel_layout)4);
  stbi_write_jpg((dir_256 / out_name).string().c_str(), 256, 256, 4, data_256, 90);
  delete[] data_256;

  unsigned char* data_64 = new unsigned char[1ll * 64 * 64 * 4];
  stbir_resize_uint8_linear(data, width, height, 0, data_64, 64, 64, 0, (stbir_pixel_layout)4);
  stbi_write_jpg((dir_64 / out_name).string().c_str(), 64, 64, 4, data_64, 90);
  delete[] data_64;

  stbi_image_free(data);

  ctx.set_result({track_id, out_name});
}

static std::filesystem::path get_cover_path(const std::filesystem::path& folder_name, db::track_id_t track_id) {
  if (filenames.size() <= track_id) { return {}; }
  if (filenames[track_id].empty()) { return {}; }
  auto& filename = filenames[track_id][0];
  if (filename.empty()) { return {}; }
  auto path = io::get_cover_cache_path() / folder_name / filename;
  if (!std::filesystem::is_regular_file(path)) {
    if (!folder_name.empty() && !filename.empty()) { std::filesystem::remove(path); }
    filenames[track_id].erase(filenames[track_id].begin());
    return {};
  }
  return path;
}

static std::filesystem::path get_cover_path_full_res(db::track_id_t track_id) {
  return get_cover_path("full", track_id);
}

static std::filesystem::path get_cover_path_256(db::track_id_t track_id) { return get_cover_path("256", track_id); }

static std::filesystem::path get_cover_path_64(db::track_id_t track_id) { return get_cover_path("64", track_id); }

static void ensure_worker_started(db::track_id_t track_id) {
  if (filenames.size() > track_id && !filenames[track_id].empty()) { return; }

  if (track_ids_in_progress.find(track_id) == track_ids_in_progress.end()) {
    track_ids_in_progress.insert(track_id);
    active_jobs.emplace_back(worker.run(track_id, db::track_by_id(track_id)->get().file_path));
  }
}

std::optional<std::filesystem::path> zincbox::cover_art::get_full_res(db::track_id_t track_id) {
  auto path = get_cover_path_full_res(track_id);
  if (path.empty()) {
    ensure_worker_started(track_id);
    return std::nullopt;
  }
  return path;
}

std::optional<std::filesystem::path> zincbox::cover_art::get_256(db::track_id_t track_id) {
  auto path = get_cover_path_256(track_id);
  if (path.empty()) {
    ensure_worker_started(track_id);
    return std::nullopt;
  }
  return path;
}

std::optional<std::filesystem::path> zincbox::cover_art::get_64(db::track_id_t track_id) {
  auto path = get_cover_path_64(track_id);
  if (path.empty()) {
    ensure_worker_started(track_id);
    return std::nullopt;
  }
  return path;
}

void zincbox::cover_art::deserialize(std::ifstream& is) {
  size_t tracks_size = 0;
  read_bin(is, tracks_size);
  if (tracks_size > 4194304) {
    out::critical("Database corrupted: invalid tracks_size");
    exit(1);
  }

  filenames.resize(tracks_size);
  for (size_t i = 0; i < tracks_size; i += 1) {
    size_t paths_size = 0;
    read_bin(is, paths_size);
    for (size_t p = 0; p < paths_size; p += 1) {
      std::string path_utf8;
      read_str(is, path_utf8);
      filenames[i].emplace_back(utf8_to_path(path_utf8));
    }
  }
}

void zincbox::cover_art::serialize(std::ofstream& os, std::span<const size_t> old_track_id_to_new_track_id) {
  std::vector<std::vector<std::filesystem::path>> new_filenames;

  new_filenames.reserve(db::track_count());

  for (size_t old_id = 0; old_id < filenames.size(); old_id += 1) {
    if (old_id >= old_track_id_to_new_track_id.size()) { break; }
    size_t new_id = old_track_id_to_new_track_id[old_id];
    if (new_id != db::INVALID_ID) { new_filenames.emplace_back(filenames[old_id]); }
  }

  write_bin(os, new_filenames.size());
  for (const auto& paths : new_filenames) {
    write_bin(os, paths.size());
    for (const auto& path : paths) {
      write_str(os, path_to_utf8(path));
    }
  }
}

void zincbox::cover_art::update() {
  decltype(active_jobs) active_jobs_;
  for (auto job_id : active_jobs) {
    auto job = worker.get(job_id);
    if (job.is_result()) {
      auto res = job.result();
      if (res) {
        auto [track_id, filename] = *res;
        if (!filename.empty()) {
          if (filenames.size() <= track_id) { filenames.resize(track_id + 1); }
          filenames[track_id].emplace_back(filename);
        }
        track_ids_in_progress.erase(track_id);
      }
    } else {
      active_jobs_.emplace_back(job_id);
    }
  }
  active_jobs = std::move(active_jobs_);
}
