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
      return (theme != prev.theme || language != prev.language || interface_scale != prev.interface_scale ||
              font_size != prev.font_size);
    }

    // General
    enum class CoverPreference : u8 { Album, Playlist };
    CoverPreference cover_preference = CoverPreference::Album;
    i32 volume_step = 2;
    // Playback
    bool shuffle_allow_same_album = false;
    bool shuffle_allow_same_artist = false;

    // Interface
    std::string theme = "default";
    std::string language = "en-US";
    i32 interface_scale = 100;
    i32 font_size = 12;

    jt::Json to_json() const {
      jt::Json json;
      json["general"]["coverPreference"] = (cover_preference == CoverPreference::Album) ? "album" : "playlist";
      json["general"]["volumeStep"] = volume_step;
      json["playback"]["shuffleAllowSameAlbum"] = shuffle_allow_same_album;
      json["playback"]["shuffleAllowSameArtist"] = shuffle_allow_same_artist;
      json["interface"]["theme"] = theme;
      json["interface"]["language"] = language;
      json["interface"]["interfaceScale"] = interface_scale;
      json["interface"]["fontSize"] = font_size;
      return json;
    }

    void from_json(const jt::Json& json) {
      if (!json.isObject()) { return; }

      if (json.contains("general") && json["general"].isObject()) {
        auto& general = json["general"];
        if (general.contains("coverPreference") && general["coverPreference"].isString()) {
          std::string cover_pref = general["coverPreference"].getString();
          if (cover_pref == "album") {
            cover_preference = CoverPreference::Album;
          } else if (cover_pref == "playlist") {
            cover_preference = CoverPreference::Playlist;
          }
        }
        if (general.contains("volumeStep") && general["volumeStep"].isNumber()) {
          volume_step = std::clamp(general["volumeStep"].getNumber(), 1.0, 10.0);
        }
      }

      if (json.contains("playback") && json["playback"].isObject()) {
        auto& playback = json["playback"];
        if (playback.contains("shuffleAllowSameAlbum") && playback["shuffleAllowSameAlbum"].isBool()) {
          shuffle_allow_same_album = playback["shuffleAllowSameAlbum"].getBool();
        }
        if (playback.contains("shuffleAllowSameArtist") && playback["shuffleAllowSameArtist"].isBool()) {
          shuffle_allow_same_artist = playback["shuffleAllowSameArtist"].getBool();
        }
      }

      if (json.contains("interface") && json["interface"].isObject()) {
        auto& interface = json["interface"];
        if (interface.contains("theme") && interface["theme"].isString()) { theme = interface["theme"].getString(); }
        if (interface.contains("language") && interface["language"].isString()) {
          language = interface["language"].getString();
        }
        if (interface.contains("interfaceScale") && interface["interfaceScale"].isNumber()) {
          interface_scale = std::clamp<i32>(interface["interfaceScale"].getNumber(), 50, 200);
        }
        if (interface.contains("fontSize") && interface["fontSize"].isNumber()) {
          font_size = std::clamp<i32>(interface["fontSize"].getNumber(), 8, 32);
        }
      }
    }
};
