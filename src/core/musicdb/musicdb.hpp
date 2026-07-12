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

  void set_playlists_collection_name(std::u32string_view);
  void set_loved_tracks_playlist_name(std::u32string_view);

  // getters
  std::optional<track_info> find_track_from_json(const jt::Json&);

  // collection getters
  std::optional<std::reference_wrapper<const Collection>> collection_by_id(collection_id_t);
  const std::vector<Collection>& all_collections();
  size_t collection_count();
  playlist_id_t playlist_loved_tracks_id();
  Playlist& playlist_loved_tracks();
  std::optional<collection_id_t> collection_of_playlist(playlist_id_t);

  // collection setters
  collection_id_t add_collection(std::u32string_view);
  playlist_id_t add_playlist_to_collection(collection_id_t, Playlist);
  void mark_collection_as_tombstone(collection_id_t);
  bool add_path_to_collection(collection_id_t, std::string_view path);
  bool remove_path_from_collection(collection_id_t, std::string_view path);
  void rescan_collection(collection_id_t);
  void rename_collection(collection_id_t, std::u32string_view new_name);

  // playlist getters
  std::optional<std::reference_wrapper<const Playlist>> playlist_by_id(playlist_id_t);
  std::optional<playlist_id_t> playlist_id_by_path(const fs::path&);
  std::vector<playlist_id_t> playlist_ids_by_name(std::u32string_view);
  const std::vector<Playlist>& all_playlists();
  size_t playlist_count();
  playlist_id_t get_album_id(collection_id_t, std::u32string album_name, std::u32string album_artist,
                             std::u32string_view file_path);

  // playlist setters
  void mark_playlist_as_tombstone(playlist_id_t);
  bool add_track_id_to_playlist(playlist_id_t, track_id_t);
  bool remove_track_id_from_playlist(playlist_id_t, track_id_t);
  bool remove_track_index_from_playlist(playlist_id_t, size_t track_index);
  track_id_t add_track_to_playlist(playlist_id_t, Track&);
  void set_playlist_image(playlist_id_t, std::string_view image_path);
  void reset_playlist_image(playlist_id_t);
  void rename_playlist(playlist_id_t, std::u32string_view new_name);
  void sort_playlist_by_track_number(playlist_id_t);
  void sort_playlist_by_artist_asc(playlist_id_t);
  void sort_playlist_by_artist_desc(playlist_id_t);
  void sort_playlist_by_name_asc(playlist_id_t);
  void sort_playlist_by_name_desc(playlist_id_t);

  // track getters
  std::optional<std::reference_wrapper<const Track>> track_by_id(track_id_t);
  std::unordered_set<track_id_t> track_by_title(std::u32string_view);
  std::unordered_set<track_id_t> track_by_artist_title(std::u32string_view, std::u32string_view);
  std::optional<track_id_t> track_by_path(std::u32string_view);
  const std::vector<Track>& all_tracks();
  size_t track_count();

  // track setters
  void set_track_playback_error(track_id_t, bool playback_error);

  // search
  std::vector<playlist_info> search_playlists(std::u32string_view search_text, size_t max_size);
  std::vector<playlist_info> search_playlists(std::u32string_view search_text, collection_id_t, size_t max_size);
  std::vector<playlist_id_t> search_playlists(std::u32string_view search_text, std::span<playlist_id_t>,
                                              size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, collection_id_t, size_t max_size);
  std::vector<track_info> search_tracks(std::u32string_view search_text, std::span<track_info> src, size_t max_size);
} // namespace db
