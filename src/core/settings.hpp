#pragma once
#include <algorithm>
#include <string>
#include "common/types.hpp"
#include "lib/json.cpp/json.h"

struct settings {
  public:
    settings() = default;
    settings(const settings&) = default;
    settings& operator=(const settings&) = default;

    static settings& get() {
      static settings instance;
      return instance;
    }

    bool operator==(const settings&) const = default;
    bool must_reload(const settings& prev) const {
      return (theme != prev.theme || language != prev.language || scale != prev.scale || font_size != prev.font_size);
    }

    // General
    enum class CoverPreference : u8 { Album, Playlist };
    CoverPreference cover_preference = CoverPreference::Album;
    i32 volume_step = 2;
    // Playback
    bool shuffle_allow_same_album = false;
    bool shuffle_allow_same_artist = false;
    bool restart_on_previous = true;
    // Interface
    std::string theme = "default";
    std::string language = "en-US";
    i32 scale = 100;
    i32 font_size = 12;
    float scrolling_speed = 40.0f;

    jt::Json to_json() const {
      jt::Json json;
      json["general"]["cover_preference"] = (cover_preference == CoverPreference::Album) ? "album" : "playlist";
      json["general"]["volume_step"] = volume_step;
      json["playback"]["shuffle_allow_same_album"] = shuffle_allow_same_album;
      json["playback"]["shuffle_allow_same_artist"] = shuffle_allow_same_artist;
      json["playback"]["restart_on_previous"] = restart_on_previous;
      json["interface"]["theme"] = theme;
      json["interface"]["language"] = language;
      json["interface"]["scale"] = scale;
      json["interface"]["font_size"] = font_size;
      json["interface"]["scrolling_speed"] = scrolling_speed;
      return json;
    }

    void from_json(const jt::Json& json) {
      if (!json.isObject()) { return; }

      if (json["general"].isObject()) {
        auto& general = json["general"];
        if (general.contains("cover_preference") && general["cover_preference"].isString()) {
          std::string cover_pref = general["cover_preference"].getString();
          if (cover_pref == "album") {
            cover_preference = CoverPreference::Album;
          } else if (cover_pref == "playlist") {
            cover_preference = CoverPreference::Playlist;
          }
        }
        if (general.contains("volume_step") && general["volume_step"].isNumber()) {
          volume_step = std::clamp(general["volume_step"].getNumber(), 1.0, 10.0);
        }
      }

      if (json["playback"].isObject()) {
        auto& playback = json["playback"];
        if (playback.contains("shuffle_allow_same_album") && playback["shuffle_allow_same_album"].isBool()) {
          shuffle_allow_same_album = playback["shuffle_allow_same_album"].getBool();
        }
        if (playback.contains("shuffle_allow_same_artist") && playback["shuffle_allow_same_artist"].isBool()) {
          shuffle_allow_same_artist = playback["shuffle_allow_same_artist"].getBool();
        }
        if (playback.contains("restart_on_previous") && playback["restart_on_previous"].isBool()) {
          restart_on_previous = playback["restart_on_previous"].getBool();
        }
      }

      if (json["interface"].isObject()) {
        auto& interface = json["interface"];
        if (interface.contains("theme") && interface["theme"].isString()) { theme = interface["theme"].getString(); }
        if (interface.contains("language") && interface["language"].isString()) {
          language = interface["language"].getString();
        }
        if (interface.contains("scale") && interface["scale"].isNumber()) {
          scale = std::clamp<i32>(interface["scale"].getNumber(), 50, 200);
        }
        if (interface.contains("font_size") && interface["font_size"].isNumber()) {
          font_size = std::clamp<i32>(interface["font_size"].getNumber(), 8, 32);
        }
        if (interface.contains("scrolling_speed") && interface["scrolling_speed"].isNumber()) {
          scrolling_speed = std::clamp<i32>(interface["scrolling_speed"].getNumber(), 10, 150);
        }
      }
    }
};
