#pragma once
#include <optional>
#include <variant>
#include <vector>
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"

namespace player {

  struct QueueTrack {
      db::track_info track_info{};
  };

  struct QueuePlaylist {
    protected:
      std::vector<db::track_info> tracks;
      std::vector<bool> tracks_played;
      std::vector<size_t> playback;
      std::optional<size_t> playback_i;
      bool frozen = false;
      db::collection_id_t collection_id;
      std::optional<db::playlist_id_t> playlist_id;

    public:
      QueuePlaylist(db::collection_id_t, db::playlist_id_t);
      QueuePlaylist(db::collection_id_t);

      void assert_valid();
      std::optional<size_t> current();
      void next();
      void prev();
      bool is_start() const;
      bool is_end() const;
      void rewind();
      void seek_to_end();
      std::optional<QueuePlaylist> splice();
      std::optional<db::track_info> get_track() const {
        if (!playback_i) { return std::nullopt; }
        auto tracks_i = playback[*playback_i];
        return tracks[tracks_i];
      }
      std::vector<db::track_info> get_playback() const {
        std::vector<db::track_info> ret;
        ret.reserve(playback.size());
        for (size_t tracks_i : playback) {
          ret.emplace_back(tracks[tracks_i]);
        }
        return ret;
      }
      std::optional<db::playlist_id_t> get_playlist_id() const { return playlist_id; }
      db::playlist_id_t get_collection_id() const { return collection_id; }
      std::optional<size_t> get_playback_index() const { return playback_i; }
      size_t get_size() const {
        if (playlist_id.value_or(db::INVALID_ID) != db::INVALID_ID) {
          return db::playlist_by_id(playlist_id.value())->get().get_tracks_count();
        } else {
          return 0;
        }
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
