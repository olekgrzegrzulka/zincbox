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
    enum class ViewType {
      NONE,
      COLLECTION,
    };

    PanelTracks(UI& ui_);

    void draw() override;
    void recreate();
    void scroll_to_playlist(size_t playlist_id);
    void scroll_to_now_playing_album();
    void update() override;
    using Sprite::handle_event;
    void handle_event(Input::InputEventMouseScroll&) override;
    void clear();
    float get_scroll_px() const;
    void set_scroll_px(float px);

  protected:
    void recreate(std::optional<size_t> collection_id_);
    void create_playlist(size_t playlist_id, i32 album_start_px);

  public:
    ViewType view_type = ViewType::NONE;
    std::optional<size_t> collection_id{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, WidgetTrack* widget)> on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, WidgetTrack* widget)> on_track_rmb{};

  protected:
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    i32 old_width{};
    i32 max_scroll_px{};
    ScrollBar* scrollbar{};
    ToolTip* button_play_tooltip{};
    ToolTip* button_play_next_tooltip{};
    std::vector<std::pair<i32, size_t>> album_scroll_px;
    std::vector<WidgetAlbum*> visible_album_widgets;
};
