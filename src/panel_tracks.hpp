#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <glm/common.hpp>
#include "input.hpp"
#include "types.hpp"
#include "ui/panel.hpp"

class ScrollBar;
class WidgetAlbum;

class PanelTracks : public Panel {
  public:
    enum class ViewType {
      NONE,
      COLLECTION,
      PLAYLISTS,
    };

    PanelTracks(UI& ui_);

    void draw() override;
    void recreate();
    void scroll_to_playlist(size_t playlist_id);
    void scroll_to_album(size_t album_index_sorted);
    void scroll_to_now_playing_album();
    void update() override;
    void handle_event(Input::InputEventMouseScroll& e) override;
    void clear();

  protected:
    void recreate(std::optional<size_t> collection_id_);
    void recreate_playlist();
    void create_album(size_t album_id, i32 album_start_px);
    void create_playlist(size_t playlist_id, i32 album_start_px);

  public:
    ViewType view_type = ViewType::NONE;
    std::optional<size_t> collection_id{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index, Widget* widget)> on_track_rmb{};

  protected:
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    i32 old_width{};
    i32 max_scroll_px{};
    ScrollBar* scrollbar{};
    std::vector<std::pair<i32, size_t>> album_scroll_px;
    std::vector<WidgetAlbum*> visible_album_widgets;
};
