#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <glm/common.hpp>
#include "common/input.hpp"
#include "common/types.hpp"
#include "ui/panel_tracks_track.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/tooltip.hpp"

class ScrollBar;
class WidgetAlbum;

class PanelTracks : public Sprite {
  public:
    PanelTracks(UI& ui_);

    void draw() override;
    void recreate(std::optional<size_t> collection_id_);
    void scroll_to_playlist(size_t playlist_id);
    void scroll_to_track(size_t playlist_id, size_t track_id);
    void update() override;
    using Sprite::event;
    void event(Input::InputEventMouseScroll&) override;
    void clear();
    float get_scroll_px() const;
    void set_scroll_px(float px);

  protected:
    void create_playlist(size_t playlist_id, i32 album_start_px);

  public:
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack* widget)>
      on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack* widget)>
      on_track_rmb{};

    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_more_options_invoked{};
    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_sort_button_pressed{};

  protected:
    std::optional<size_t> collection_id{};
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    i32 old_width{};
    i32 max_scroll_px{};
    ScrollBar* scrollbar{};
    ToolTip* button_play_tooltip{};
    ToolTip* button_play_next_tooltip{};
    ToolTip* button_sort_tooltip{};
    ToolTip* button_more_tooltip{};
    std::vector<std::pair<i32, size_t>> album_scroll_px;
    std::vector<WidgetAlbum*> visible_album_widgets;
};
