#include "collection.hpp"
#include "common/serialize.hpp"
#include "core/musicdb/musicdb.hpp"

db::Collection::Collection(std::ifstream& is) {
  read_str(is, name);

  size_t paths_size = 0;
  read_bin(is, paths_size);
  for (size_t i = 0; i < paths_size; i += 1) {
    std::string path;
    read_str(is, path);
    paths.emplace(path);
  }

  size_t playlist_ids_size = 0;
  read_bin(is, playlist_ids_size);
  playlist_ids.resize(playlist_ids_size);
  for (size_t i = 0; i < playlist_ids_size; i += 1) {
    size_t value;
    read_bin(is, value);
    playlist_ids[i] = value;
  }
}

bool db::Collection::add_path(fs::path path) {
  if (playlist_ids.size() > 0 && playlist_ids[0] == 0) { return false; }
  paths.emplace(path.string());
  return true;
}

std::optional<size_t> db::Collection::next_playlist_id(size_t playlist_id) const {
  auto index = find_playlist_index(playlist_id);
  if (index.has_value() && index.value() + 1 < playlist_ids.size()) {
    return playlist_ids[index.value() + 1];
  } else {
    return std::nullopt;
  }
}

std::optional<size_t> db::Collection::prev_playlist_id(size_t playlist_id) const {
  auto index = find_playlist_index(playlist_id);
  if (index.has_value() && index.value() > 0) {
    return playlist_ids[index.value() - 1];
  } else {
    return std::nullopt;
  }
}

void db::Collection::serialize(std::ofstream& os) const {
  write_str(os, name);

  write_bin(os, paths.size());
  for (const auto& path : paths) {
    write_str(os, path);
  }

  std::vector<size_t> nontombstoned_playlist_ids;
  for (size_t playlist_id : playlist_ids) {
    auto& playlist = db::playlist_by_id(playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }
    nontombstoned_playlist_ids.emplace_back(playlist_id);
  }
  write_bin(os, nontombstoned_playlist_ids.size());
  for (size_t playlist_id : nontombstoned_playlist_ids) {
    write_bin(os, playlist_id);
  }
}

void db::Collection::serialize(std::ofstream& os, const std::vector<size_t>& old_playlist_id_to_new_playlist_id) const {
  write_str(os, name);

  write_bin(os, paths.size());
  for (const auto& path : paths) {
    write_str(os, path);
  }

  std::vector<size_t> nontombstoned_playlist_ids;
  for (size_t old_playlist_id : playlist_ids) {
    size_t new_playlist_id = old_playlist_id_to_new_playlist_id[old_playlist_id];
    if (new_playlist_id == db::playlist_count()) { continue; }
    auto& playlist = db::playlist_by_id(old_playlist_id)->get();
    if (playlist.is_tombstone()) { continue; }
    nontombstoned_playlist_ids.emplace_back(new_playlist_id);
  }
  write_bin(os, nontombstoned_playlist_ids.size());
  for (size_t playlist_id : nontombstoned_playlist_ids) {
    write_bin(os, playlist_id);
  }
}

std::optional<size_t> db::Collection::find_playlist_index(size_t playlist_id) const {
  for (size_t i = 0; i < playlist_ids.size(); i += 1) {
    if (playlist_ids[i] == playlist_id) { return i; }
  }
  return std::nullopt;
}
