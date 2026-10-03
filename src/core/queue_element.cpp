#include "core/queue_element.hpp"
#include <optional>
#include <variant>
#include <vector>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/random.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"

namespace player {

  bool player::QueueElement::is_track() const noexcept { return std::holds_alternative<QueueTrack>(*this); }

  bool player::QueueElement::is_playlist() const noexcept { return std::holds_alternative<QueuePlaylist>(*this); }

  bool player::QueueElement::is_collection() const noexcept { return std::holds_alternative<QueueCollection>(*this); }

  QueueTrack* player::QueueElement::track() { return std::get_if<QueueTrack>(this); }
  QueuePlaylist* player::QueueElement::playlist() { return std::get_if<QueuePlaylist>(this); }
  QueueCollection* player::QueueElement::collection() { return std::get_if<QueueCollection>(this); }

  const QueueTrack* player::QueueElement::track() const { return std::get_if<QueueTrack>(this); }
  const QueuePlaylist* player::QueueElement::playlist() const { return std::get_if<QueuePlaylist>(this); }
  const QueueCollection* player::QueueElement::collection() const { return std::get_if<QueueCollection>(this); }

  player::QueuePlaylist::QueuePlaylist(db::collection_id_t collection_id_, db::playlist_id_t playlist_id_) {
    collection_id = collection_id_;
    playlist_id = playlist_id_;
    auto playlist = db::playlist_by_id(*playlist_id);
    ensure(playlist);
    tracks.reserve(playlist->get().get_tracks_count());
    for (auto track_id : playlist->get().track_ids) {
      tracks.emplace_back(
        db::track_info{.collection_id = collection_id, .playlist_id = *playlist_id, .track_id = track_id});
    }
    tracks_played.resize(tracks.size(), false);

    next();
  }

  player::QueuePlaylist::QueuePlaylist(db::collection_id_t collection_id_) {
    collection_id = collection_id_;
    auto collection = db::collection_by_id(collection_id);
    ensure(collection);

    for (auto playlist_id : collection->get().playlist_ids()) {
      auto playlist = db::playlist_by_id(playlist_id);
      ensure(playlist);
      for (auto track_id : playlist->get().track_ids) {
        tracks.emplace_back(
          db::track_info{.collection_id = collection_id, .playlist_id = playlist_id, .track_id = track_id});
      }
    }
    tracks_played.resize(tracks.size(), false);

    next();
  }

  void player::QueuePlaylist::assert_valid() { ensure(tracks.size() == tracks_played.size()); }

  std::optional<size_t> player::QueuePlaylist::current() {
    assert_valid();
    if (!playback_i.has_value()) { return std::nullopt; }

    if (*playback_i < playback.size()) { return playback[*playback_i]; }

    return std::nullopt;
  }

  void player::QueuePlaylist::next() {
    assert_valid();

    if (playback_i.has_value()) {
      if (*playback_i + 1 < playback.size()) {
        *playback_i += 1;
        return;
      }
    }

    if (frozen) {
      playback_i = std::nullopt;
      return;
    }

    auto shuffle = player::get_shuffle_mode();
    auto repeat = player::get_repeat_mode();
    auto curr = current();

    if (repeat == player::RepeatMode::TRACK && curr.has_value()) {
      push_track(*curr);
      return;
    } else if (repeat == player::RepeatMode::OFF) {
      if (shuffle == player::ShuffleMode::ON) {
        next_shuffle(false);
        return;
      } else {
        next_not_played(false);
        return;
      }
    } else if (repeat == player::RepeatMode::ALBUM) {
      if (shuffle == player::ShuffleMode::ON) {
        next_shuffle(true);
        return;
      } else {
        next_not_played(true);
        return;
      }
    }
  }

  void player::QueuePlaylist::prev() {
    assert_valid();
    if (playback_i.has_value()) {
      if (*playback_i > 0) { *playback_i -= 1; }
    }
  }

  bool player::QueuePlaylist::is_start() const { return playback_i == 0; }

  bool player::QueuePlaylist::is_end() const { return !playback_i.has_value(); }

  void player::QueuePlaylist::rewind() {
    if (playback.empty()) {
      playback_i.reset();
    } else {
      playback_i = 0;
    }
  }

  void player::QueuePlaylist::seek_to_end() {
    if (playback.empty()) {
      playback_i.reset();
    } else {
      playback_i = playback.size() - 1;
    }
  }

  std::optional<player::QueuePlaylist> player::QueuePlaylist::splice() {
    QueuePlaylist tail(*this);
    tail.frozen = false;

    if (playback_i.has_value()) {
      size_t split_idx = *playback_i + 1;
      if (split_idx < playback.size()) {
        tail.playback = std::vector<size_t>(playback.begin() + split_idx, playback.end());
      } else {
        tail.playback.clear();
      }
      playback.erase(playback.begin() + split_idx, playback.end());
    } else {
      tail.playback.clear();
    }

    tail.playback_i = 0;
    this->frozen = true;

    return tail;
  }

  void player::QueuePlaylist::clear() { std::fill(tracks_played.begin(), tracks_played.end(), false); }

  void player::QueuePlaylist::next_not_played(bool loop_back) {
    if (tracks.empty()) {
      playback_i = std::nullopt;
      return;
    }

    size_t start = 0;
    size_t end = 0;
    auto curr = current();

    if (curr.has_value()) {
      start = *curr + 1;
      end = *curr;
    }

    for (size_t i = start; i < tracks.size(); ++i) {
      if (!tracks_played[i]) {
        push_track(i);
        return;
      }
    }

    for (size_t i = 0; i < end; ++i) {
      if (!tracks_played[i]) {
        push_track(i);
        return;
      }
    }

    if (loop_back) {
      clear();
      push_track(0);
    } else {
      playback_i = std::nullopt;
    }
  }

  void player::QueuePlaylist::next_shuffle(bool loop_back) {
    if (tracks.empty()) {
      playback_i = std::nullopt;
      return;
    }

    std::vector<size_t> unplayed;
    for (size_t i = 0; i < tracks_played.size(); ++i) {
      if (!tracks_played[i]) { unplayed.push_back(i); }
    }

    if (unplayed.empty()) {
      if (loop_back) {
        clear();
        for (size_t i = 0; i < tracks_played.size(); ++i) {
          unplayed.push_back(i);
        }
      } else {
        playback_i = std::nullopt;
        return;
      }
    }

    size_t best_candidate = unplayed[0];
    int best_score = -1000000;
    auto& rng = StaticRandom::get();
    size_t tries = std::min<size_t>(static_cast<size_t>(5), unplayed.size());

    for (size_t t = 0; t < tries; ++t) {
      size_t cand_idx = rng.next<size_t>(0, unplayed.size() - 1);
      size_t cand = unplayed[cand_idx];

      int score = 10000;
      for (size_t i = 0; i < playback.size(); ++i) {
        if (playback[i] == cand) {
          score = -static_cast<int>(i);
          break;
        }
      }

      if (score > best_score) {
        best_score = score;
        best_candidate = cand;
      }
    }

    push_track(best_candidate);
  }

  void player::QueuePlaylist::push_track(size_t track_idx) {
    if (playback_i.has_value()) {
      if (*playback_i < playback.size() - 1) { playback.erase(playback.begin() + *playback_i + 1, playback.end()); }
    }

    playback.push_back(track_idx);
    playback_i = playback.size() - 1;

    if (track_idx < tracks_played.size()) { tracks_played[track_idx] = true; }
  }

} // namespace player
