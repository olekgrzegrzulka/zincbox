#pragma once
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace db {

  struct Collection final {
    public:
      std::u32string name;
      std::vector<size_t> playlist_ids;
      bool tombstone = false;
      std::unordered_set<std::string> paths;

    public:
      Collection(std::u32string_view name_) { name = name_; }
      Collection(std::ifstream&);
      bool add_path(const fs::path&);
      bool remove_path(const fs::path&);
      size_t add_playlist(size_t playlist_id) {
        playlist_ids.emplace_back(playlist_id);
        return playlist_ids.size() - 1;
      }
      std::optional<size_t> next_playlist_id(size_t playlist_id) const;
      std::optional<size_t> prev_playlist_id(size_t playlist_id) const;
      void set_tombstone(bool t) { tombstone = t; }
      bool is_tombstone() const { return tombstone; }
      bool has_playlist(size_t playlist_id) const {
        return std::find(playlist_ids.begin(), playlist_ids.end(), playlist_id) != playlist_ids.end();
      }

      void serialize(std::ofstream&) const;
      void serialize(std::ofstream&, const std::vector<size_t>& old_playlist_id_to_new_playlist_id) const;

    protected:
      std::optional<size_t> find_playlist_index(size_t playlist_id) const;
  };
} // namespace db
