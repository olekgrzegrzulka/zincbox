#pragma once
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "collection.hpp"
#include "core/musicdb/types.hpp"
#include "lib/json.cpp/json.h"
#include "playlist.hpp"
#include "track.hpp"

namespace fs = std::filesystem;

namespace db {
  void create_empty_db();
  void serialize(std::ofstream&);
  void deserialize(std::ifstream&);
  void print_collections();

  struct track_info {
      size_t collection_id = INVALID_ID;
      size_t playlist_id = INVALID_ID;
      size_t track_id = INVALID_ID;

      auto operator<=>(const track_info&) const = default;
  };

  void set_playlists_collection_name(std::u32string_view);
  void set_loved_tracks_playlist_name(std::u32string_view);

  // getters
  std::optional<track_info> find_track_from_json(const jt::Json&);

  // collection getters
  std::optional<std::reference_wrapper<const Collection>> collection_by_id(size_t);
  const std::vector<Collection>& all_collections();
  size_t collection_count();
  size_t playlist_loved_tracks_id();
  Playlist& playlist_loved_tracks();
  std::optional<size_t> collection_of_playlist(size_t playlist_id);

  // collection setters
  size_t add_collection(std::u32string_view);
  size_t add_playlist_to_collection(size_t collection_id, Playlist);
  void mark_collection_as_tombstone(size_t);
  bool add_path_to_collection(size_t collection_id, std::string_view path);
  bool remove_path_from_collection(size_t collection_id, std::string_view path);
  void rescan_collection(size_t collection_id);
  void rename_collection(size_t collection_id, std::u32string_view new_name);

  // playlist getters
  std::optional<std::reference_wrapper<const Playlist>> playlist_by_id(size_t);
  std::optional<size_t> playlist_id_by_path(const fs::path&);
  std::vector<size_t> playlist_ids_by_name(std::u32string_view);
  const std::vector<Playlist>& all_playlists();
  size_t playlist_count();
  size_t get_album_id(size_t collection_id, std::u32string album_name, std::u32string album_artist,
                      std::u32string_view file_path);

  // playlist setters
  void mark_playlist_as_tombstone(size_t);
  bool add_track_id_to_playlist(size_t playlist_id, size_t track_id);
  bool remove_track_id_from_playlist(size_t playlist_id, size_t track_id);
  bool remove_track_index_from_playlist(size_t playlist_id, size_t track_index);
  size_t add_track_to_playlist(size_t playlist_id, Track&);
  void set_playlist_image(size_t playlist_id, std::string_view image_path);
  void reset_playlist_image(size_t playlist_id);
  void rename_playlist(size_t playlist_id, std::u32string_view new_name);
  void sort_playlist_by_track_number(size_t playlist_id);
  void sort_playlist_by_artist_asc(size_t playlist_id);
  void sort_playlist_by_artist_desc(size_t playlist_id);
  void sort_playlist_by_name_asc(size_t playlist_id);
  void sort_playlist_by_name_desc(size_t playlist_id);

  // track getters
  std::optional<std::reference_wrapper<const Track>> track_by_id(size_t);
  std::unordered_set<size_t> track_by_title(std::u32string_view);
  std::unordered_set<size_t> track_by_artist_title(std::u32string_view, std::u32string_view);
  std::optional<size_t> track_by_path(std::u32string_view);
  const std::vector<Track>& all_tracks();
  size_t track_count();

  // track setters
  void set_track_playback_error(size_t track_id, bool playback_error);

  // search

  struct playlist_info {
      size_t collection_id;
      size_t playlist_id;
  };
  std::vector<playlist_info> search_playlists(std::u32string_view search_text, size_t max_size);
  std::vector<playlist_info> search_playlists(std::u32string_view search_text, size_t collection_id, size_t max_size);
  std::vector<size_t> search_playlists(std::u32string_view search_text, std::span<size_t> playlist_ids,
                                       size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, size_t collection_id, size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, std::span<track_info> src, size_t max_size);
} // namespace db
