#pragma once
#include <string>
#include <glaze/glaze.hpp>
#include "common/color.hpp"
#include "common/types.hpp"

template <> struct glz::meta<rgba> {
    static constexpr auto write_rgb = [](const rgba& v) -> std::string {
      return "rgb(" + std::to_string(static_cast<i32>(v.r)) + "," + std::to_string(static_cast<i32>(v.g)) + "," +
             std::to_string(static_cast<i32>(v.b)) + ")";
    };

    static constexpr auto read_rgb = [](rgba& v, const std::string& s) {
      if (auto c = color_utils::parse_color(s)) { v = c.value(); }
    };

    static constexpr auto value = glz::custom<read_rgb, write_rgb>;
};

struct ButtonConfig {
    bool visible{true};
    i32 width{36};
    i32 nine_slice_margin{0};
};

struct TopBarConfig {
    rgba color{0x1e0e1c};
    i32 height{26};
    ButtonConfig button_add_tab{};
};

struct SeekbarConfig {
    bool visible{true};
    i32 nine_slice_margin{6};
    i32 track_height{12};
    i32 thumb_width{12};
    i32 thumb_height{12};
};

struct VolumeBarConfig {
    bool visible{true};
    i32 width{70};
    i32 nine_slice_margin{6};
    i32 track_height{12};
    i32 thumb_width{12};
    i32 thumb_height{12};
};

struct TimestampConfig {
    bool visible{true};
};

struct PanelControlsConfig {
    rgba color{0x1e0e1c};
    i32 height{44};
    i32 padding{4};
    ButtonConfig button_previous;
    ButtonConfig button_play;
    ButtonConfig button_stop;
    ButtonConfig button_next;
    SeekbarConfig seekbar;
    TimestampConfig timestamp;
    VolumeBarConfig volume_bar;
    ButtonConfig button_repeat;
    ButtonConfig button_shuffle;
};

struct PanelTracklistConfig {
    rgba color{0x0f070c};
    i32 track_height{22};
    i32 header_height{28};
    i32 header_spacing{10};
    rgba header_author_color{0xa8769eff};
    rgba header_name_color{0xdc99ceff};
    rgba track_color_odd{0x0c050bff};
    rgba track_color_even{0x150913ff};
    rgba track_artist_color{0x80667aff};
    rgba track_number_color{0xa6859fff};
    rgba title_color{0xcca3c4ff};
    rgba length_color{0x80667aff};
};

struct PanelPlaylistsConfig {
    rgba color{0x0f070c};
    rgba title_color{0xdc99ceff};
    rgba author_color{0xa8769eff};
};

struct NotificationConfig {
    i32 nine_slice_margin{6};
    i32 height{10};
    i32 spacing{10};
};

struct ThemeConfig {
    rgba text_color{0xf2c2e9ff};
    rgba text_color_muted{0xb388aaff};
    rgba text_color_disabled{0x998a96ff};
    TopBarConfig top_bar;
    PanelControlsConfig panel_controls;
    PanelTracklistConfig panel_tracklist;
    PanelPlaylistsConfig panel_playlists;
    NotificationConfig notification;
};
