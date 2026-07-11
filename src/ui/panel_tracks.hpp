#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <glm/common.hpp>
#include "common/input.hpp"
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "ui/widget_track.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/tooltip.hpp"

class ScrollBar;
class WidgetPlaylistHeader;

class PanelTracksSelection {
  public:
    bool has(db::track_info s) const { return selection_set.contains(s); }
    void insert(db::track_info s) {
      if (selection_set.emplace(s).second) { selection_vector.emplace_back(s); }
    }
    void erase(db::track_info s) {
      if (selection_set.erase(s)) {
        std::erase_if(selection_vector, [&s](db::track_info s_) -> bool { return s == s_; });
      }
    }

    bool empty() const { return selection_vector.empty(); }

    void clear() {
      selection_set.clear();
      selection_vector.clear();
    }

    std::optional<db::track_info> back() const {
      if (selection_vector.empty()) { return std::nullopt; }
      return selection_vector.back();
    }

    const std::vector<db::track_info>& get() const { return selection_vector; }

  private:
    std::set<db::track_info> selection_set; // FIXME unordered_set
    std::vector<db::track_info> selection_vector;
};

class PanelTracks final : public Sprite {
  public:
    PanelTracks(UI& ui_);
    ~PanelTracks();

    void draw() override;
    void recreate(std::optional<size_t> collection_id_);
    void scroll_to_playlist(size_t playlist_id);
    void scroll_to_track(size_t playlist_id, size_t track_id);
    void update() override;
    using Sprite::event;
    void event(Input::InputEventMouseScroll&) override;
    void event(Input::InputEventMouseButton&) override;
    void clear();
    float get_scroll_px() const;
    void set_scroll_px(float px);
    const PanelTracksSelection& selection() { return m_selection; }

  protected:
    PanelTracksSelection m_selection;

  public:
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack*)>
      on_track_lmb{};
    std::function<void(size_t collection_id, size_t playlist_id, size_t track_id, size_t playlist_track_index,
                       WidgetTrack*)>
      on_track_rmb{};

    std::function<void(WidgetTrack*)> on_selection_rmb{};

    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_more_options_invoked{};
    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_sort_button_pressed{};

  protected:
    std::optional<size_t> collection_id{};
    double scroll_px{};
    double target_scroll_px{};
    double old_scroll_px{};
    vec2i old_size{};
    i32 max_scroll_px{};
    bool just_recreated = false;
    bool selection_modified = false;
    ScrollBar* scrollbar{};
    ToolTip* button_play_tooltip{};
    ToolTip* button_play_next_tooltip{};
    ToolTip* button_sort_tooltip{};
    ToolTip* button_more_tooltip{};

    enum class ElementType : u8 { TRACK, HEADER };
    struct element {
        ElementType type{};
        db::track_info track_info;

        bool is_track() const { return true; }
        bool is_header() const { return false; }

        i32 height() const {
          static const i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
          static const i32 header_height = theme::get_prop("tracklist_playlist_header_height").as_i32();
          if (type == ElementType::TRACK) {
            return track_height;
          } else if (type == ElementType::HEADER) {
            return header_height;
          }
          return 0;
        }
    };
    void create_element(std::pair<element, Widget*>&, size_t track_number);
    std::vector<std::pair<element, Widget*>> elements;
    Widget* elements_container{};
};
