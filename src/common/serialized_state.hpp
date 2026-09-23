#pragma once
#include <optional>
#include <string>
#include <vector>
#include <glaze/glaze.hpp>
#include "common/color.hpp"
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/track.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "core/settings.hpp"

template <> struct glz::meta<rgba> {
    static constexpr auto read = [](rgba& v, const std::string& s) {
      if (auto c = color_utils::parse_color(s)) { v = c.value(); }
    };
    static constexpr auto write = [](const rgba& v) -> std::string {
      return "rgb(" + std::to_string(static_cast<i32>(v.r)) + "," + std::to_string(static_cast<i32>(v.g)) + "," +
             std::to_string(static_cast<i32>(v.b)) + ")";
    };
    static constexpr auto value = glz::custom<read, write>;
};

template <> struct glz::meta<player::RepeatMode> {
    using enum player::RepeatMode;
    static constexpr auto value = enumerate("off", OFF, "track", TRACK, "album", ALBUM);
};

template <> struct glz::meta<player::ShuffleMode> {
    using enum player::ShuffleMode;
    static constexpr auto value = enumerate("off", OFF, "on", ON);
};

struct QueueTrackSerialized {
    std::string collection;
    std::string playlist;
    std::optional<i32> track_number;
    std::string title;
    std::string artist;
    std::vector<std::string> album_artist;
    std::string genre;
    std::optional<i32> year;
    i32 bitrate_kb_s;
    i32 duration_ms;
    std::string path;

    QueueTrackSerialized() = default;
    QueueTrackSerialized(db::track_info ti) {
      auto track_ = db::track_by_id(ti.track_id);
      auto playlist_ = db::playlist_by_id(ti.playlist_id);
      auto collection_ = db::collection_by_id(ti.collection_id);
      if (!track_ || !playlist_ || !collection_) { return; }
      auto& track = track_->get();

      collection = collection_->get().name();
      playlist = playlist_->get().name;
      track_number = track.metadata.track_number;
      title = track.metadata.title;
      artist = track.metadata.artist;
      album_artist = track.metadata.album_artist;
      genre = track.metadata.genre;
      year = track.metadata.year;
      bitrate_kb_s = track.metadata.bitrate_kb_s;
      duration_ms = track.metadata.duration_ms;
      path = path_to_utf8(track.file_path);
    }
};

template <> struct glz::meta<QueueTrackSerialized> {
    using T = QueueTrackSerialized;
    static constexpr auto value =
      glz::object("collection", &T::collection, "playlist", &T::playlist, "track_number", &T::track_number, "title",
                  &T::title, "artist", &T::artist, "album_artist", &T::album_artist, "genre", &T::genre, "year",
                  &T::year, "bitrate_kb_s", &T::bitrate_kb_s, "duration_ms", &T::duration_ms, "path", &T::path);
};

struct PlaylistTrackSerialized {
    std::optional<i32> track_number;
    std::string title;
    std::string artist;
    std::vector<std::string> album_artist;
    std::string genre;
    std::optional<i32> year;
    i32 bitrate_kb_s;
    i32 duration_ms;
    std::string path;

    PlaylistTrackSerialized() = default;
    PlaylistTrackSerialized(const db::Track& track) {
      track_number = track.metadata.track_number;
      title = track.metadata.title;
      artist = track.metadata.artist;
      album_artist = track.metadata.album_artist;
      genre = track.metadata.genre;
      year = track.metadata.year;
      bitrate_kb_s = track.metadata.bitrate_kb_s;
      duration_ms = track.metadata.duration_ms;
      path = path_to_utf8(track.file_path);
    }
};

template <> struct glz::meta<PlaylistTrackSerialized> {
    using T = PlaylistTrackSerialized;
    static constexpr auto value =
      glz::object("track_number", &T::track_number, "title", &T::title, "artist", &T::artist, "album_artist",
                  &T::album_artist, "genre", &T::genre, "year", &T::year, "bitrate_kb_s", &T::bitrate_kb_s,
                  "duration_ms", &T::duration_ms, "path", &T::path);
};

struct PlaylistSerialized {
    std::string title;
    std::vector<PlaylistTrackSerialized> tracks;

    PlaylistSerialized() = default;
    PlaylistSerialized(const db::Playlist& playlist) {
      title = playlist.name;
      for (auto track_id : playlist.track_ids) {
        auto& track = db::track_by_id(track_id)->get();
        tracks.emplace_back(PlaylistTrackSerialized(track));
      }
    }
};

template <> struct glz::meta<PlaylistSerialized> {
    using T = PlaylistSerialized;
    static constexpr auto value = glz::object("title", &T::title, "tracks", &T::tracks);
};

struct PlayerSerialized {
    player::RepeatMode repeat_mode;
    player::ShuffleMode shuffle_mode;
    float volume{1.0f};
    i64 timestamp{0};
    std::optional<i32> queue_index;
    std::vector<QueueTrackSerialized> queue;
};

struct InterfaceSerialized {
    bool mini_player = false;
    i32 playlists_scroll_offset = 0;
    std::string selected_tab{};
    std::vector<std::string> tabs_order{};
    i32 tracks_scroll_offset = 0;
    i32 window_width = 0;
    i32 window_height = 0;
    bool window_maximized = false;
};

struct AppSerialized {
    PlayerSerialized player;
    Settings settings;
    InterfaceSerialized interface;
};

template <> struct glz::meta<Settings::CoverPreference> {
    using enum Settings::CoverPreference;
    static constexpr auto value = glz::enumerate("album", Album, "playlist", Playlist);
};
