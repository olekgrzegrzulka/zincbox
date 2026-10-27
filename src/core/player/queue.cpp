#include "core/player/queue.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include "common/logger.hpp"
#include "common/random.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/settings.hpp"
#include "zincbox.hpp"

namespace {

  enum class PendingType : u8 {
    NONE,
    REPEAT_CURRENT_TRACK,
    STEP_PLAYLIST_ELEMENT,
    STEP_COLLECTION_ELEMENT,
    ADVANCE_QUEUE_INDEX,
    APPEND_NEW_TRACK
  };

  struct PendingNext {
      PendingType type = PendingType::NONE;
      db::track_info track{};
  };

  Random rng{};
  std::vector<player::QueueElement> playing_queue{};
  std::optional<size_t> playing_index{};
  player::ShuffleMode shuffle_mode = player::ShuffleMode::OFF;
  player::RepeatMode repeat_mode = player::RepeatMode::OFF;
  PendingNext pending_next{};

  std::optional<i32> get_track_history_distance(size_t track_id, i32 from_index, i32 to_index) {
    if (playing_queue.empty()) { return std::nullopt; }
    from_index = std::max(from_index, 0);
    to_index = std::min(to_index, static_cast<i32>(playing_queue.size()) - 1);
    if (from_index > to_index) { return std::nullopt; }
    for (i32 i = to_index; i >= from_index; i -= 1) {
      if (playing_queue[i].is_track() && track_id == playing_queue[i].track()->track_info.track_id) {
        return to_index - i;
      }
    }
    return std::nullopt;
  }
} // namespace

namespace player::queue {

  void cancel_pending_next() { pending_next = PendingNext{}; }

  bool has_pending_next() { return pending_next.type != PendingType::NONE; }

  bool set_or_insert_current(const QueueElement& elem, bool clear_queue) {
    cancel_pending_next();
    if (clear_queue || playing_queue.empty()) {
      playing_queue = {elem};
      playing_index = 0;
      signal_on_queue_changed.emit(false);
    } else if (!playing_index.has_value() || playing_index.value() >= playing_queue.size() - 1) {
      playing_queue.emplace_back(elem);
      playing_index = playing_queue.size() - 1;
      signal_on_queue_changed.emit(true);
    } else {
      playing_queue[*playing_index + 1] = elem;
      (*playing_index) += 1;
      signal_on_queue_changed.emit(false);
    }
    return true;
  }

  bool play_playlist(size_t collection_id, size_t playlist_id, bool clear_queue) {
    cancel_pending_next();
    auto playlist = db::playlist_by_id(playlist_id);
    if (!playlist.has_value()) { return false; }
    auto track_ids = playlist->get().get_track_ids();
    if (track_ids.empty()) { return false; }

    bool do_play = clear_queue || playing_queue.empty();

    if (clear_queue) {
      playing_queue.clear();
      playing_queue = {QueuePlaylist(collection_id, playlist_id)};
      playing_index = 0;
      signal_on_queue_changed.emit(false);
    } else {
      size_t insert_at = playing_index.has_value() ? playing_index.value() + 1 : playing_queue.size();
      queue::enqueue(QueuePlaylist(collection_id, playlist_id), insert_at - 1);
    }

    return do_play;
  }

  bool enqueue(const QueueElement& elem, size_t at) {
    cancel_pending_next();
    if (at >= playing_queue.size()) {
      playing_queue.emplace_back(elem);
      signal_on_queue_changed.emit(true);
    } else {
      playing_queue.emplace(playing_queue.begin() + at + 1, elem);
      signal_on_queue_changed.emit(false);
    }

    if (!playing_index.has_value()) {
      playing_index = playing_queue.size() - 1;
      return true;
    }
    return false;
  }

  std::pair<std::vector<size_t>, bool> add_tracks(std::span<const db::track_info> tracks, size_t at) {
    cancel_pending_next();
    if (tracks.empty()) { return {{}, false}; }

    bool appended = (at >= playing_queue.size());
    size_t insert_pos = appended ? playing_queue.size() : at;

    std::vector<QueueElement> to_insert;
    to_insert.reserve(tracks.size());
    for (const auto& t : tracks) {
      to_insert.emplace_back(QueueTrack{t});
    }

    playing_queue.insert(playing_queue.begin() + insert_pos, to_insert.begin(), to_insert.end());
    signal_on_queue_changed.emit(appended);

    if (playing_index.has_value() && playing_index.value() >= insert_pos) {
      playing_index = playing_index.value() + tracks.size();
    }

    bool should_start_playback = false;
    if (!playing_index.has_value()) {
      playing_index = insert_pos;
      should_start_playback = true;
    }

    std::vector<size_t> ret(tracks.size());
    std::iota(ret.begin(), ret.end(), insert_pos);
    return {ret, should_start_playback};
  }

  bool remove_at(size_t at) {
    cancel_pending_next();
    if (at >= playing_queue.size()) { return false; }
    bool current_track_removed = (at == playing_index);
    if (playing_index.has_value() && playing_index <= at) {
      playing_queue.erase(playing_queue.begin() + at);
    } else if (playing_index.has_value() && playing_index > at) {
      playing_queue.erase(playing_queue.begin() + at);
      playing_index = playing_index.value() - 1;
    } else {
      playing_queue.erase(playing_queue.begin() + at);
    }

    if (playing_queue.empty()) {
      playing_index = std::nullopt;
    } else if (playing_index.has_value()) {
      playing_index = std::min(playing_index.value(), playing_queue.size() - 1);
    }

    return current_track_removed;
  }

  bool remove_indices(std::span<const size_t> indices) {
    cancel_pending_next();
    if (indices.empty() || playing_queue.empty()) { return false; }

    std::vector<size_t> sorted_indices(indices.begin(), indices.end());
    std::sort(sorted_indices.begin(), sorted_indices.end(), std::greater<size_t>());
    sorted_indices.erase(std::unique(sorted_indices.begin(), sorted_indices.end()), sorted_indices.end());

    bool current_track_removed = false;

    for (size_t at : sorted_indices) {
      if (at >= playing_queue.size()) { continue; }
      if (playing_index.has_value() && at == playing_index.value()) { current_track_removed = true; }
      playing_queue.erase(playing_queue.begin() + at);
      if (playing_index.has_value() && playing_index.value() > at) { playing_index = playing_index.value() - 1; }
    }

    if (playing_queue.empty()) {
      playing_index = std::nullopt;
    } else if (playing_index.has_value()) {
      playing_index = std::min(playing_index.value(), playing_queue.size() - 1);
    }

    return current_track_removed;
  }

  std::vector<size_t> move_tracks(std::span<const size_t> indices, size_t target_index) {
    cancel_pending_next();
    if (indices.empty() || playing_queue.empty()) { return {}; }

    std::set<size_t, std::less<size_t>> sorted_indices(indices.begin(), indices.end());

    if (*sorted_indices.begin() >= playing_queue.size() || *sorted_indices.rbegin() >= playing_queue.size()) {
      out::error("player::move_queue_tracks(): index out of bounds");
      return {};
    }

    if (sorted_indices.size() != indices.size()) {
      out::error("player::move_queue_tracks(): duplicate indices");
      return {};
    }

    std::vector<QueueElement> tracks_to_insert;
    tracks_to_insert.reserve(indices.size());
    std::optional<size_t> playing_track_offset;
    for (size_t i = 0; i < indices.size(); ++i) {
      if (playing_index == indices[i]) { playing_track_offset = i; }
      tracks_to_insert.emplace_back(playing_queue[indices[i]]);
    }

    std::optional<size_t> corrected_playing_index;
    if (playing_index.has_value() && !playing_track_offset.has_value()) {
      auto it = sorted_indices.lower_bound(playing_index.value());
      corrected_playing_index = playing_index.value() - std::distance(sorted_indices.begin(), it);
    }

    auto it_target = sorted_indices.lower_bound(target_index);
    size_t corrected_target_index = target_index - std::distance(sorted_indices.begin(), it_target);

    std::vector<QueueElement> playing_queue_new;
    playing_queue_new.reserve(playing_queue.size() - sorted_indices.size());
    for (size_t i = 0; i < playing_queue.size(); ++i) {
      if (sorted_indices.contains(i)) { continue; }
      playing_queue_new.emplace_back(playing_queue[i]);
    }

    playing_queue = std::move(playing_queue_new);
    playing_queue.insert(playing_queue.begin() + corrected_target_index, tracks_to_insert.begin(),
                         tracks_to_insert.end());

    if (playing_track_offset.has_value()) {
      playing_index = corrected_target_index + playing_track_offset.value();
    } else if (corrected_playing_index.has_value()) {
      if (corrected_playing_index.value() >= corrected_target_index) {
        corrected_playing_index.value() += tracks_to_insert.size();
      }
      playing_index = std::min(corrected_playing_index.value(), playing_queue.size() - 1);
    }

    std::vector<size_t> ret(tracks_to_insert.size());
    std::iota(ret.begin(), ret.end(), corrected_target_index);
    return ret;
  }

  void clear() {
    cancel_pending_next();
    playing_queue.clear();
    playing_index = std::nullopt;
    signal_on_queue_changed.emit(false);
  }

  std::optional<db::track_info> prepare_next_track() {
    cancel_pending_next();
    if (!playing_index.has_value() || playing_index.value() >= playing_queue.size()) { return std::nullopt; }

    if (repeat_mode == RepeatMode::TRACK) {
      auto cur = get_playing_track();
      if (cur.has_value()) {
        pending_next = PendingNext{
          .type = PendingType::REPEAT_CURRENT_TRACK,
          .track = *cur,
        };
        return cur;
      }
      return std::nullopt;
    }

    auto& current_elem = playing_queue[*playing_index];
    if (current_elem.is_playlist()) {
      auto* pl = current_elem.playlist();
      auto index_opt = pl->peek_next();
      if (index_opt) {
        auto next_ti = pl->get_queue_element(index_opt);
        if (next_ti) {
          pending_next = PendingNext{
            .type = PendingType::STEP_PLAYLIST_ELEMENT,
            .track = *next_ti,
          };
          return next_ti;
        }
        return std::nullopt;
      }
    } else if (current_elem.is_collection()) {
      auto* pl = current_elem.collection();
      auto index_opt = pl->peek_next();
      if (index_opt) {
        auto next_ti = pl->get_queue_element(index_opt);
        if (next_ti) {
          pending_next = PendingNext{
            .type = PendingType::STEP_COLLECTION_ELEMENT,
            .track = *next_ti,
          };
          return next_ti;
        }
        return std::nullopt;
      }
    }

    if (playing_index.value() < playing_queue.size() - 1) {
      auto& next = playing_queue[*playing_index + 1];

      std::optional<db::track_info> next_ti;
      if (next.is_track()) {
        next_ti = next.track()->track_info;
      } else if (next.is_playlist()) {
        next_ti = next.playlist()->get_queue_element(0);
      } else if (next.is_collection()) {
        next_ti = next.collection()->get_queue_element(0);
      }

      if (next_ti.has_value()) {
        pending_next = PendingNext{
          .type = PendingType::ADVANCE_QUEUE_INDEX,
          .track = *next_ti,
        };
        return next_ti;
      }
      return std::nullopt;
    }

    if (current_elem.is_playlist() || current_elem.is_collection()) { return std::nullopt; }

    auto playing = current_elem.track()->track_info;
    if (!db::track_by_id(playing.track_id) || !db::playlist_by_id(playing.playlist_id) ||
        !db::collection_by_id(playing.collection_id)) {
      return std::nullopt;
    }

    auto& track = db::track_by_id(playing.track_id)->get();
    auto& playlist = db::playlist_by_id(playing.playlist_id)->get();
    auto& collection = db::collection_by_id(playing.collection_id)->get();

    if (shuffle_mode == ShuffleMode::OFF) {
      auto next_track_id = playlist.next_track_id(playing.track_id);
      if (next_track_id.has_value()) {
        db::track_info ti{
          .collection_id = playing.collection_id,
          .playlist_id = playing.playlist_id,
          .track_id = next_track_id.value(),
        };
        pending_next = PendingNext{
          .type = PendingType::APPEND_NEW_TRACK,
          .track = ti,
        };
        return ti;
      }
      if (repeat_mode == RepeatMode::OFF) {
        std::optional<db::playlist_id_t> playlist_id = playing.playlist_id;
        for (i32 i = 0; i < 10; i += 1) {
          if (!playlist_id) { break; }
          playlist_id = collection.next_playlist_id(*playlist_id);
          if (!playlist_id) { break; }
          auto playlist = db::playlist_by_id(*playlist_id);
          if (!playlist) { break; }
          if (playlist->get().get_tracks_count() == 0) { continue; }
          db::track_info ti{
            .collection_id = playing.collection_id,
            .playlist_id = *playlist_id,
            .track_id = playlist->get().get_track_ids()[0],
          };
          pending_next = PendingNext{
            .type = PendingType::APPEND_NEW_TRACK,
            .track = ti,
          };
          return ti;
        }
      } else if (repeat_mode == RepeatMode::ALBUM) {
        if (playlist.get_tracks_count() == 0) {
          out::debug_warn("player::play: empty playlist");
          return std::nullopt;
        }
        db::track_info ti{
          .collection_id = playing.collection_id,
          .playlist_id = playing.playlist_id,
          .track_id = playlist.get_track_ids()[0],
        };
        pending_next = PendingNext{
          .type = PendingType::APPEND_NEW_TRACK,
          .track = ti,
        };
        return ti;
      }
    } else if (shuffle_mode == ShuffleMode::ON) {
      if (repeat_mode == RepeatMode::OFF) {
        std::map<i32, db::track_info> candidates;
        for (i32 shuffle_tries = 10; shuffle_tries > 0; shuffle_tries -= 1) {
          auto& playlist_ids = db::collection_by_id(playing.collection_id)->get().playlist_ids();
          size_t rand_playlist_id = rng.pick(std::span(playlist_ids));
          i32 collection_track_count = 0;
          for (size_t pid : playlist_ids) {
            collection_track_count += db::playlist_by_id(pid)->get().get_tracks_count();
          }
          size_t rand_track_id = rng.pick(std::span{db::playlist_by_id(rand_playlist_id)->get().get_track_ids()});
          i32 last_tracks_to_check = std::clamp(collection_track_count / 2, 0, 30);
          i32 score = get_track_history_distance(rand_track_id,
                                                 static_cast<i32>(playing_index.value_or(0)) - last_tracks_to_check,
                                                 static_cast<i32>(playing_index.value_or(0)))
                        .value_or(last_tracks_to_check);

          auto candidate = db::track_info{
            .collection_id = playing.collection_id,
            .playlist_id = rand_playlist_id,
            .track_id = rand_track_id,
          };

          if (!playing_queue.empty()) {
            bool same_playlist_as_prev = playing_queue.back().is_track() &&
                                         playing_queue.back().track()->track_info.playlist_id == rand_playlist_id;
            std::string artist_prev =
              playing_queue.back().is_track()
                ? db::track_by_id(playing_queue.back().track()->track_info.track_id)->get().metadata.artist
                : "";
            std::string_view artist_rand = db::track_by_id(rand_track_id)->get().metadata.artist;
            bool same_artist_as_prev = (artist_prev == artist_rand);
            bool allow_album = !zincbox::settings().playback.shuffle_allow_same_album || !same_playlist_as_prev;
            bool allow_artist = !zincbox::settings().playback.shuffle_allow_same_artist || !same_artist_as_prev;

            if (allow_album) { score += 3; }
            if (allow_artist) { score += 3; }

            if (allow_album && allow_artist && score - 6 > last_tracks_to_check / 2) {
              candidates = {{score, candidate}};
              break;
            }
            candidates.insert({score, candidate});

          } else {
            candidates = {{score, candidate}};
            break;
          }
        }

        if (!candidates.empty()) {
          db::track_info ti = candidates.rbegin()->second;
          pending_next = PendingNext{
            .type = PendingType::APPEND_NEW_TRACK,
            .track = ti,
          };
          return ti;
        }
      } else if (repeat_mode == RepeatMode::ALBUM) {
        std::map<i32, db::track_info> candidates;
        for (i32 shuffle_tries = 10; shuffle_tries > 0; shuffle_tries -= 1) {
          size_t rand_track_id = rng.pick(std::span{playlist.get_track_ids()});
          i32 last_tracks_to_check = std::clamp(static_cast<i32>(playlist.get_tracks_count()) / 2, 0, 30);

          i32 score = get_track_history_distance(rand_track_id,
                                                 static_cast<i32>(playing_index.value_or(0)) - last_tracks_to_check,
                                                 static_cast<i32>(playing_index.value_or(0)))
                        .value_or(last_tracks_to_check);

          auto candidate = db::track_info{
            .collection_id = playing.collection_id,
            .playlist_id = playing.playlist_id,
            .track_id = rand_track_id,
          };

          if (!playing_queue.empty()) {
            bool same_playlist_as_prev = false;
            bool same_artist_as_prev = false;
            bool is_album = (playlist.type == db::PlaylistType::ALBUM);
            if (!is_album) {
              auto prev_originating_album_id = track.originating_album_id;
              auto rand_originating_album_id = db::track_by_id(rand_track_id)->get().originating_album_id;
              same_playlist_as_prev = (prev_originating_album_id == rand_originating_album_id);
              std::string_view artist_prev = track.metadata.artist;
              std::string_view artist_rand = db::track_by_id(rand_track_id)->get().metadata.artist;
              same_artist_as_prev = (artist_prev == artist_rand);
            }
            bool allow_album = zincbox::settings().playback.shuffle_allow_same_album || !same_playlist_as_prev;
            bool allow_artist = zincbox::settings().playback.shuffle_allow_same_artist || !same_artist_as_prev;
            if (allow_album) { score += 3; }
            if (allow_artist) { score += 3; }
            if (allow_album && allow_artist && score - 6 > last_tracks_to_check / 2) {
              candidates = {{score, candidate}};
              break;
            }
            candidates.insert({score, candidate});

          } else {
            candidates = {{score, candidate}};
            break;
          }
        }

        if (!candidates.empty()) {
          db::track_info ti = candidates.rbegin()->second;
          pending_next = PendingNext{
            .type = PendingType::APPEND_NEW_TRACK,
            .track = ti,
          };
          return ti;
        }
      }
    }

    return std::nullopt;
  }

  bool commit_pending_next() {
    if (pending_next.type == PendingType::NONE) { return false; }

    PendingNext local = pending_next;
    pending_next = PendingNext{};

    if (!playing_index.has_value() || playing_index.value() >= playing_queue.size()) { return false; }

    if (local.type == PendingType::REPEAT_CURRENT_TRACK) { return true; }

    if (local.type == PendingType::STEP_PLAYLIST_ELEMENT) {
      if (playing_queue[*playing_index].is_playlist()) {
        std::get<QueuePlaylist>(playing_queue[*playing_index]).next();
        signal_on_queue_changed_at.emit(*playing_index);
        return true;
      }
    } else if (local.type == PendingType::STEP_COLLECTION_ELEMENT) {
      if (playing_queue[*playing_index].is_collection()) {
        std::get<QueueCollection>(playing_queue[*playing_index]).next();
        signal_on_queue_changed_at.emit(*playing_index);
        return true;
      }
    } else if (local.type == PendingType::ADVANCE_QUEUE_INDEX) {
      if (playing_index.value() < playing_queue.size() - 1) {
        playing_index = playing_index.value() + 1;
        if (playing_queue[*playing_index].is_playlist()) { playing_queue[*playing_index].playlist()->rewind(); }
        if (playing_queue[*playing_index].is_collection()) { playing_queue[*playing_index].collection()->rewind(); }
        return true;
      }
    } else if (local.type == PendingType::APPEND_NEW_TRACK) {
      set_or_insert_current(QueueTrack{local.track}, false);
      return true;
    }

    return false;
  }

  NavAction advance_next() {
    auto next_ti = prepare_next_track();
    if (!next_ti.has_value()) { return NavAction::NONE; }
    bool is_repeat_one = (pending_next.type == PendingType::REPEAT_CURRENT_TRACK);
    if (!commit_pending_next()) { return NavAction::NONE; }
    if (is_repeat_one) { return NavAction::SEEK_ZERO_AND_PLAY; }
    return NavAction::PLAY_CURRENT;
  }

  NavAction step_prev(i32 current_time_ms) {
    cancel_pending_next();
    if (!playing_index.has_value()) { return NavAction::NONE; }

    if (repeat_mode == RepeatMode::TRACK) { return NavAction::SEEK_ZERO; }

    if (zincbox::settings().playback.restart_on_previous && current_time_ms > 5000) { return NavAction::SEEK_ZERO; }

    auto& current_elem = playing_queue[*playing_index];
    if (auto* qp = std::get_if<QueuePlaylist>(&current_elem); qp != nullptr && !qp->is_start()) {
      qp->prev();
      signal_on_queue_changed_at.emit(*playing_index);
      return NavAction::PLAY_CURRENT;
    }

    if (auto* qc = std::get_if<QueueCollection>(&current_elem); qc != nullptr && !qc->is_start()) {
      qc->prev();
      signal_on_queue_changed_at.emit(*playing_index);
      return NavAction::PLAY_CURRENT;
    }

    if (playing_index.value() != 0) {
      playing_index = playing_index.value() - 1;
      if (playing_queue[*playing_index].is_playlist()) { playing_queue[*playing_index].playlist()->seek_to_end(); }
      if (playing_queue[*playing_index].is_collection()) { playing_queue[*playing_index].collection()->seek_to_end(); }
      return NavAction::PLAY_CURRENT;
    }

    if (current_elem.is_playlist() || current_elem.is_collection()) { return NavAction::NONE; }

    auto playing = current_elem.track()->track_info;
    auto& collection = db::collection_by_id(playing.collection_id)->get();
    auto& playlist = db::playlist_by_id(playing.playlist_id)->get();
    auto prev_track_id = playlist.prev_track_id(playing.track_id);

    std::optional<db::track_info> play_prev;

    if (prev_track_id.has_value()) {
      play_prev = db::track_info{
        .collection_id = playing.collection_id,
        .playlist_id = playing.playlist_id,
        .track_id = prev_track_id.value(),
      };
    } else if (repeat_mode == RepeatMode::OFF) {
      std::optional<db::playlist_id_t> playlist_id = playing.playlist_id;
      for (i32 i = 0; i < 10; i += 1) {
        if (!playlist_id) { break; }
        playlist_id = collection.prev_playlist_id(*playlist_id);
        if (!playlist_id) { break; }
        auto playlist = db::playlist_by_id(*playlist_id);
        if (!playlist) { break; }
        size_t count = playlist->get().get_tracks_count();
        if (count == 0) { continue; }
        play_prev = db::track_info{
          .collection_id = playing.collection_id,
          .playlist_id = *playlist_id,
          .track_id = playlist->get().get_track_ids()[count - 1],
        };
        break;
      }

      if (!play_prev.has_value()) { return NavAction::SEEK_ZERO; }
    } else if (repeat_mode == RepeatMode::ALBUM) {
      if (playlist.get_tracks_count() == 0) { return NavAction::NONE; }
      play_prev = db::track_info{
        .collection_id = playing.collection_id,
        .playlist_id = playing.playlist_id,
        .track_id = playlist.get_track_ids()[playlist.get_tracks_count() - 1],
      };
    }

    if (play_prev.has_value()) {
      playing_queue.emplace(playing_queue.begin(), QueueTrack{play_prev.value()});
      signal_on_queue_changed.emit(false);
      return NavAction::PLAY_CURRENT;
    }

    return NavAction::NONE;
  }

  std::optional<QueueElement> get_playing() {
    if (!playing_index.has_value() || playing_index.value() >= playing_queue.size()) { return std::nullopt; }
    return playing_queue[playing_index.value()];
  }

  std::optional<db::track_info> get_playing_track() {
    auto playing = get_playing();
    if (!playing.has_value()) { return std::nullopt; }
    if (playing->is_track()) { return playing->track()->track_info; }
    if (playing->is_playlist()) { return playing->playlist()->get_track(); }
    if (playing->is_collection()) { return playing->collection()->get_track(); }
    return std::nullopt;
  }

  const std::vector<QueueElement>& get_elements() { return playing_queue; }

  std::optional<size_t> get_playing_index() { return playing_index; }

  bool set_playing_index(std::optional<size_t> index) {
    cancel_pending_next();
    if (index.has_value() && index.value() >= playing_queue.size()) { return false; }
    playing_index = index;
    if (playing_index.has_value()) {
      if (playing_queue[*index].is_playlist()) { playing_queue[*index].playlist()->rewind(); }
      if (playing_queue[*index].is_collection()) { playing_queue[*index].collection()->rewind(); }
    }
    return true;
  }

  ShuffleMode get_shuffle_mode() { return shuffle_mode; }

  bool set_shuffle_mode(ShuffleMode mode) {
    cancel_pending_next();
    if (mode == shuffle_mode) { return false; }
    shuffle_mode = mode;
    return true;
  }

  RepeatMode get_repeat_mode() { return repeat_mode; }

  bool set_repeat_mode(RepeatMode mode) {
    cancel_pending_next();
    if (mode == repeat_mode) { return false; }
    repeat_mode = mode;
    return true;
  }

} // namespace player::queue
