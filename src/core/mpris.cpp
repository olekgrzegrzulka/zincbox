#include "core/mpris.hpp"
#include <optional>
#include <string>
#include "common/types.hpp"

#ifdef ZINCBOX_HAS_MPRIS
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include "common/logger.hpp"
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/sdbus-c++.h>

namespace {
  mpris::CommandQueue command_queue;
  std::unique_ptr<sdbus::IObject> mpris_object;
  std::string status = "Stopped";
  double volume = 1.0;
  std::string track_id = "/org/mpris/MediaPlayer2/Track/0";
  i64 duration_us;
  std::string title;
  std::string artist;
  std::string album;
  std::string cover_path;
  mpris::LoopStatus loop_status = mpris::LoopStatus::NONE;
  bool shuffle = false;

  const char* loop_status_to_string(mpris::LoopStatus s) {
    switch (s) {
    case mpris::LoopStatus::TRACK: return "Track";
    case mpris::LoopStatus::PLAYLIST: return "Playlist";
    case mpris::LoopStatus::NONE:
    default: return "None";
    }
  }

  mpris::LoopStatus string_to_loop_status(const std::string& val) {
    if (val == "Track") { return mpris::LoopStatus::TRACK; }
    if (val == "Playlist") { return mpris::LoopStatus::PLAYLIST; }
    return mpris::LoopStatus::NONE;
  }
} // namespace

static const char* const PLAYER_NAME = "zincbox";
static const char* const PLAYER_ID = "org.mpris.MediaPlayer2.zincbox";

void mpris::init() {
  auto thread = std::thread([]() {
    try {
      auto connection = sdbus::createSessionBusConnection(sdbus::ServiceName{PLAYER_ID});

      const sdbus::ObjectPath PATH{"/org/mpris/MediaPlayer2"};
      mpris_object = sdbus::createObject(*connection, PATH);

      mpris_object
        ->addVTable(sdbus::registerProperty("Identity").withGetter([]() { return std::string(PLAYER_NAME); }),
                    sdbus::registerProperty("CanQuit").withGetter([]() { return false; }),
                    sdbus::registerProperty("CanRaise").withGetter([]() { return false; }),
                    sdbus::registerProperty("HasTrackList").withGetter([]() { return false; }))
        .forInterface("org.mpris.MediaPlayer2");

      mpris_object
        ->addVTable(
          sdbus::registerProperty("CanGoNext").withGetter([]() { return true; }),
          sdbus::registerProperty("CanGoPrevious").withGetter([]() { return true; }),
          sdbus::registerProperty("CanPlay").withGetter([]() { return true; }),
          sdbus::registerProperty("CanPause").withGetter([]() { return true; }),
          sdbus::registerProperty("CanSeek").withGetter([]() { return true; }),
          sdbus::registerProperty("CanControl").withGetter([]() { return true; }),
          sdbus::registerProperty("PlaybackStatus").withGetter([]() { return status; }),
          sdbus::registerProperty("Volume").withGetter([]() { return volume; }),
          sdbus::registerProperty("Metadata").withGetter([]() {
            return std::map<std::string, sdbus::Variant>{
              {"mpris:trackid", sdbus::Variant(sdbus::ObjectPath{track_id})},
              {"mpris:length", sdbus::Variant(duration_us)},
              {"xesam:title", sdbus::Variant(title)},
              {"xesam:artist", sdbus::Variant(std::vector<std::string>{artist})},
              {"xesam:album", sdbus::Variant(album)},
              {"mpris:artUrl", sdbus::Variant(std::string{cover_path})}};
          }),
          sdbus::registerProperty("LoopStatus")
            .withGetter([]() { return std::string(loop_status_to_string(loop_status)); })
            .withSetter([](const std::string& val) {
              if (val == "None" || val == "Track" || val == "Playlist") {
                loop_status = string_to_loop_status(val);
                command_queue.push(Command{.type = CommandType::LOOP, .value = (i32)(loop_status)});
              }
            }),

          sdbus::registerProperty("Shuffle").withGetter([]() { return shuffle; }).withSetter([](const bool& val) {
            shuffle = val;
            command_queue.push(Command{.type = CommandType::SHUFFLE, .value = val});
          }),
          sdbus::registerMethod("Quit").implementedAs([]() {}),
          sdbus::registerMethod("Next").implementedAs([]() { command_queue.push(Command{.type = CommandType::NEXT}); }),
          sdbus::registerMethod("Previous").implementedAs([]() {
            command_queue.push(Command{.type = CommandType::PREVIOUS});
          }),
          sdbus::registerMethod("Play").implementedAs([]() { command_queue.push(Command{.type = CommandType::PLAY}); }),
          sdbus::registerMethod("PlayPause").implementedAs([]() {
            command_queue.push(Command{.type = CommandType::PLAY_PAUSE});
          }),
          sdbus::registerMethod("Pause").implementedAs(
            []() { command_queue.push(Command{.type = CommandType::PAUSE}); }),
          sdbus::registerMethod("Stop").implementedAs([]() { command_queue.push(Command{.type = CommandType::STOP}); }),

          sdbus::registerMethod("Seek").withInputParamNames("Offset").implementedAs([](i64 us) {
            i64 ms = us / 1000;
            command_queue.push(Command{.type = CommandType::SEEK, .value = ms});
          }),

          sdbus::registerMethod("SetPosition")
            .withInputParamNames("TrackId", "Position")
            .implementedAs([](const sdbus::ObjectPath&, i64 us) {
              i64 ms = us / 1000;
              command_queue.push(Command{.type = CommandType::SET, .value = ms});
            }))
        .forInterface("org.mpris.MediaPlayer2.Player");

      connection->enterEventLoop();

    } catch (const sdbus::Error& e) { out::error("d-bus error: {} - {} ", e.getName(), e.getMessage()); }
  });
  thread.detach();
}

void mpris::deinit() {}

std::optional<mpris::Command> mpris::command_pop() { return command_queue.pop(); }

void mpris::notify_playback_status_playing() {
  status = "Playing";
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
  }
}

void mpris::notify_playback_status_paused() {
  status = "Paused";
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
  }
}

void mpris::notify_playback_status_stopped() {
  status = "Stopped";
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
  }
}

void mpris::notify_track_change(std::string title_, std::string artist_, std::string album_, i64 duration_ms,
                                std::string cover_path_) {
  title = std::move(title_);
  artist = std::move(artist_);
  album = std::move(album_);
  duration_us = duration_ms * 1000;
  cover_path = "file://" + std::move(cover_path_);
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Metadata")});
  }
}

void mpris::notify_cover_change(std::string cover_path_) {
  std::string cover_path_temp = "file://" + std::move(cover_path_);
  if (cover_path != cover_path_temp) {
    cover_path = std::move(cover_path_temp);
    if (mpris_object) {
      mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Metadata")});
    }
  }
}

void mpris::notify_volume(double volume_) {
  volume = volume_;
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Volume")});
  }
}

void mpris::notify_seeked(i64 position_ms) {
  if (mpris_object) {
    mpris_object->emitSignal("Seeked").onInterface("org.mpris.MediaPlayer2.Player").withArguments(position_ms * 1000);
  }
}

void mpris::notify_loop_status(i32 status_) {
  if (status_ > static_cast<i32>(LoopStatus::PLAYLIST)) { return; }

  loop_status = static_cast<LoopStatus>(status_);
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("LoopStatus")});
  }
}

void mpris::notify_shuffle(bool state_) {
  shuffle = state_;
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Shuffle")});
  }
}

#else
void mpris::init() {}
void mpris::deinit() {}
std::optional<mpris::Command> mpris::command_pop() { return std::nullopt; }
void mpris::notify_playback_status_playing() {}
void mpris::notify_playback_status_paused() {}
void mpris::notify_playback_status_stopped() {}
void mpris::notify_track_change(std::string, std::string, std::string, i64, std::string) {}
void mpris::notify_cover_change(std::string) {}
void mpris::notify_volume(double) {}
void mpris::notify_seeked(i64) {}
void mpris::notify_loop_status(i32) {}
void mpris::notify_shuffle(bool) {}
#endif
