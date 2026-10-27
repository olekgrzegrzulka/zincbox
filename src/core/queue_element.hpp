#pragma once
#include <optional>
#include <variant>
#include <vector>
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"

namespace player {

  struct QueueTrack final {
      db::track_info track_info{};
  };

  struct QueuePlaylist {
    private:
      std::vector<db::track_info> m_tracks;
      std::vector<bool> m_tracks_played;
      std::vector<size_t> m_playback;
      std::optional<size_t> m_playback_i;
      bool m_frozen = false;
      db::collection_id_t m_collection_id;
      std::optional<db::playlist_id_t> m_playlist_id;

    public:
      QueuePlaylist(db::collection_id_t, db::playlist_id_t);
      QueuePlaylist(db::collection_id_t);

      void assert_valid();
      std::optional<size_t> current();
      void next();
      [[nodiscard]] std::optional<size_t> peek_next();
      void prev();
      bool is_start() const;
      bool is_end() const;
      void rewind();
      void seek_to_end();
      std::optional<QueuePlaylist> splice();
      std::optional<db::track_info> get_track() const {
        if (!m_playback_i) { return std::nullopt; }
        auto tracks_i = m_playback[*m_playback_i];
        return m_tracks[tracks_i];
      }
      std::vector<db::track_info> get_playback() const {
        std::vector<db::track_info> ret;
        ret.reserve(m_playback.size());
        for (size_t tracks_i : m_playback) {
          ret.emplace_back(m_tracks[tracks_i]);
        }
        return ret;
      }

      std::optional<db::track_info> get_queue_element(std::optional<size_t> queue_index) const {
        if (!queue_index) { return std::nullopt; }
        if (*queue_index >= m_playback.size()) { return std::nullopt; }
        size_t track_index = m_playback[*queue_index];
        if (track_index >= m_tracks.size()) { return std::nullopt; }
        return m_tracks[track_index];
      }

      std::optional<db::playlist_id_t> get_playlist_id() const { return m_playlist_id; }
      db::playlist_id_t get_collection_id() const { return m_collection_id; }
      std::optional<size_t> get_playback_index() const { return m_playback_i; }
      size_t get_size() const {
        if (m_playlist_id.value_or(db::INVALID_ID) != db::INVALID_ID) {
          return db::playlist_by_id(m_playlist_id.value())->get().get_tracks_count();
        }
        return 0;
      }

    private:
      void clear();
      void next_not_played(bool loop_back);
      void next_shuffle(bool loop_back);
      void push_track(size_t track_idx);
  };

  struct QueueCollection : public QueuePlaylist {};

  struct QueueElement : public std::variant<QueueTrack, QueuePlaylist, QueueCollection> {
      using variant::variant;
      bool is_track() const noexcept;
      bool is_playlist() const noexcept;
      bool is_collection() const noexcept;
      QueueTrack* track();
      QueuePlaylist* playlist();
      QueueCollection* collection();
      const QueueTrack* track() const;
      const QueuePlaylist* playlist() const;
      const QueueCollection* collection() const;
  };
} // namespace player
