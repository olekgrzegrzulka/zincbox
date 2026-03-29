#include <map>
#include <string>
#include "debug.hpp"
#include "mpris.hpp"
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/sdbus-c++.h>

static mpris::CommandQueue command_queue;

static std::unique_ptr<sdbus::IObject> mpris_object;
static std::string status = "Stopped";
static double volume = 1.0;
static std::string track_id = "/org/mpris/MediaPlayer2/Track/0";
static i64 duration_us;
static std::string title;
static std::string artist;
static std::string album;

static const char* const player_name = "Player name goes here";
static const char* const player_id = "org.mpris.MediaPlayer2.player_name_goes_here";

void mpris::init() {
  auto thread = std::thread([]() {
    try {
      auto connection = sdbus::createSessionBusConnection(sdbus::ServiceName{player_id});

      const sdbus::ObjectPath path{"/org/mpris/MediaPlayer2"};
      mpris_object = sdbus::createObject(*connection, path);

      mpris_object->addVTable(
                    sdbus::registerProperty("Identity").withGetter([]() { return std::string(player_name); }),
                    sdbus::registerProperty("CanQuit").withGetter([]() { return false; }),
                    sdbus::registerProperty("CanRaise").withGetter([]() { return false; }),
                    sdbus::registerProperty("HasTrackList").withGetter([]() { return false; }))
        .forInterface("org.mpris.MediaPlayer2");

      mpris_object->addVTable(
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
                        {"xesam:album", sdbus::Variant(album)}};
                    }),
                    sdbus::registerMethod("Quit").implementedAs([]() {}),
                    sdbus::registerMethod("Next").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::NEXT});
                    }),
                    sdbus::registerMethod("Previous").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::PREVIOUS});
                    }),
                    sdbus::registerMethod("Play").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::PLAY});
                    }),
                    sdbus::registerMethod("PlayPause").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::PLAY_PAUSE});
                    }),
                    sdbus::registerMethod("Pause").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::PAUSE});
                    }),
                    sdbus::registerMethod("Stop").implementedAs([]() {
                      command_queue.push(Command{.type = CommandType::STOP});
                    }),

                    sdbus::registerMethod("Seek")
                      .withInputParamNames("Offset")
                      .implementedAs([](int64_t us) {
                        i64 ms = us / 1000;
                        command_queue.push(Command{.type = CommandType::SEEK, .value = ms});
                      }),

                    sdbus::registerMethod("SetPosition")
                      .withInputParamNames("TrackId", "Position")
                      .implementedAs([](const sdbus::ObjectPath&, int64_t us) {
                        i64 ms = us / 1000;
                        command_queue.push(Command{.type = CommandType::SET, .value = ms});
                      }))
        .forInterface("org.mpris.MediaPlayer2.Player");

      connection->enterEventLoop();

    } catch (const sdbus::Error& e) {
      debug_warn("d-bus error: ", e.getName(), " - ", e.getMessage());
    }
  });
  thread.detach();
}

void mpris::deinit() {
}

std::optional<mpris::Command> mpris::command_pop() {
  return command_queue.pop();
}

void mpris::notify_playback_status_playing() {
  status = "Playing";
  if (mpris_object) mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
}

void mpris::notify_playback_status_paused() {
  status = "Paused";
  if (mpris_object) mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
}

void mpris::notify_playback_status_stopped() {
  status = "Stopped";
  if (mpris_object) mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("PlaybackStatus")});
}

void mpris::notify_track_change(std::string title_, std::string artist_, std::string album_, i64 duration_ms) {
  title = std::move(title_);
  artist = std::move(artist_);
  album = std::move(album_);
  duration_us = duration_ms * 1000;
  if (mpris_object) {
    mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Metadata")});
  }
}

void mpris::notify_volume(double volume_) {
  volume = volume_;
  if (mpris_object) mpris_object->emitPropertiesChangedSignal("org.mpris.MediaPlayer2.Player", {sdbus::PropertyName("Volume")});
}

void mpris::notify_seeked(i64 position_ms) {
  if (mpris_object) {
    mpris_object->emitSignal("Seeked").onInterface("org.mpris.MediaPlayer2.Player").withArguments(position_ms * 1000);
  }
}
