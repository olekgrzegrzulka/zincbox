#pragma once
#include <algorithm>
#include <array>
#include <string>
#include "common/types.hpp"

struct Settings {
  public:
    enum class CoverPreference : u8 { ALBUM, PLAYLIST };

    bool must_reload(const Settings& other) const {
      return (interface.theme != other.interface.theme || interface.language != other.interface.language ||
              interface.scale != other.interface.scale || interface.font_size != other.interface.font_size);
    }

    void clamp_values() {
      general.volume_step = std::clamp(general.volume_step, 1, 10);
      playback.crossfade = std::clamp(playback.crossfade, 0, 5000);
      for (float& gain : playback.equalizer) {
        gain = std::clamp(gain, -12.0f, 12.0f);
      }
      interface.scale = std::clamp(interface.scale, 75, 200);
      interface.font_size = std::clamp(interface.font_size, 8, 32);
      interface.scrolling_speed = std::clamp(interface.scrolling_speed, 10.0f, 150.0f);
      interface.cover_size = std::clamp(interface.cover_size, 16, 256);
    }

    bool operator==(const Settings&) const = default;

    struct General {
        CoverPreference cover_preference = CoverPreference::ALBUM;
        i32 volume_step = 2;
        bool operator==(const General&) const = default;
    } general;

    struct Playback {
        bool shuffle_allow_same_album = false;
        bool shuffle_allow_same_artist = false;
        bool restart_on_previous = true;
        i32 crossfade = 500;
        std::array<float, 10> equalizer{};
        bool operator==(const Playback&) const = default;
    } playback;

    struct Interface {
        std::string theme = "default";
        std::string language = "en-US";
        i32 scale = 100;
        i32 font_size = 14;
        float scrolling_speed = 40.0f;
        i32 cover_size = 64;
        bool operator==(const Interface&) const = default;
    } interface;
};
