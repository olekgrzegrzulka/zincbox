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
    protected:
      std::u32string m_name;
      std::vector<size_t> m_playlist_ids;
      bool m_tombstone = false;
      std::unordered_set<std::string> m_paths;

    public:
      Collection(std::u32string_view name_) { m_name = name_; }
      Collection(std::ifstream&);
      std::u32string_view name() const { return m_name; }
      const std::vector<size_t>& playlist_ids() const { return m_playlist_ids; }
      const std::unordered_set<std::string>& paths() const { return m_paths; }
      void set_name(std::u32string_view name_) { m_name = name_; }
      bool has_path(const fs::path& path) const { return m_paths.find(path.string()) != m_paths.end(); }
      bool add_path(const fs::path&);
      bool remove_path(const fs::path&);
      size_t add_playlist(size_t playlist_id) {
        m_playlist_ids.emplace_back(playlist_id);
        return m_playlist_ids.size() - 1;
      }
      std::optional<size_t> next_playlist_id(size_t playlist_id) const;
      std::optional<size_t> prev_playlist_id(size_t playlist_id) const;
      void set_tombstone(bool t) { m_tombstone = t; }
      bool is_tombstone() const { return m_tombstone; }
      bool has_playlist(size_t playlist_id) const {
        return std::find(m_playlist_ids.begin(), m_playlist_ids.end(), playlist_id) != m_playlist_ids.end();
      }

      void serialize(std::ofstream&) const;
      void serialize(std::ofstream&, const std::vector<size_t>& old_playlist_id_to_new_playlist_id) const;

    protected:
      std::optional<size_t> find_playlist_index(size_t playlist_id) const;
  };
} // namespace db
