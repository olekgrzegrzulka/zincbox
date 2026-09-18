#pragma once
#include <algorithm>
#include <string>
#include "common/types.hpp"

struct Settings {
  public:
    enum class CoverPreference : u8 { Album, Playlist };

    bool must_reload(const Settings& other) const {
      return (interface.theme != other.interface.theme || interface.language != other.interface.language ||
              interface.scale != other.interface.scale || interface.font_size != other.interface.font_size);
    }

    void clamp_values() {
      general.volume_step = std::clamp(general.volume_step, 1, 10);
      interface.scale = std::clamp(interface.scale, 75, 200);
      interface.font_size = std::clamp(interface.font_size, 8, 32);
      interface.scrolling_speed = std::clamp(interface.scrolling_speed, 10.0f, 150.0f);
    }

    struct General {
        CoverPreference cover_preference = CoverPreference::Album;
        i32 volume_step = 2;
    } general;

    struct Playback {
        bool shuffle_allow_same_album = false;
        bool shuffle_allow_same_artist = false;
        bool restart_on_previous = true;
    } playback;

    struct Interface {
        std::string theme = "default";
        std::string language = "en-US";
        i32 scale = 100;
        i32 font_size = 14;
        float scrolling_speed = 40.0f;
    } interface;
};
