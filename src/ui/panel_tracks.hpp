#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <glm/common.hpp>
#include "common/input.hpp"
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "ui/widget_playlist_header.hpp"
#include "ui/widget_track.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/tooltip.hpp"
#include "ui_generic/widget.hpp"

class ScrollBar;

class PanelTracksSelection {
  public:
    bool has(db::track_info s) const { return selection_set.contains(s); }
    void insert(db::track_info s) {

      if (selection_set.emplace(s).second) {
        if (selection_vector.empty()) {
          common_playlist_id = s.playlist_id;
        } else if (common_playlist_id != s.playlist_id) {
          common_playlist_id = std::nullopt;
        }
        selection_vector.emplace_back(s);
      }
    }
    void erase(db::track_info s) {
      if (selection_set.erase(s)) {
        std::erase_if(selection_vector, [&s](db::track_info s_) -> bool { return s == s_; });
      }
      common_playlist_id =
        selection_vector.empty() ? std::nullopt : std::make_optional(selection_vector[0].playlist_id);
      for (auto& s2 : selection_vector) {
        if (s2.playlist_id != common_playlist_id) {
          common_playlist_id = std::nullopt;
          break;
        }
      }
    }

    bool empty() const { return selection_vector.empty(); }

    void clear() {
      selection_set.clear();
      selection_vector.clear();
      common_playlist_id = std::nullopt;
    }

    std::optional<db::track_info> back() const {
      if (selection_vector.empty()) { return std::nullopt; }
      return selection_vector.back();
    }

    const std::vector<db::track_info>& get() const { return selection_vector; }

    std::optional<db::playlist_id_t> get_common_playlist_id() const { return common_playlist_id; }

    size_t size() const { return selection_vector.size(); }

  private:
    std::optional<db::playlist_id_t> common_playlist_id{};
    std::set<db::track_info> selection_set; // FIXME unordered_set
    std::vector<db::track_info> selection_vector;
};

class PanelTracks final : public Sprite {
  public:
    using Sprite::event;

    enum class ItemType : u8 { TRACK, HEADER };
    struct Item {
        ItemType type{};
        db::track_info track_info;
        WidgetTrack* widget_track{};
        WidgetPlaylistHeader* widget_header{};

        bool is_track() const { return true; }
        bool is_header() const { return false; }
        bool has_widget() const { return widget_track || widget_header; }
        void delete_widget() {
          if (widget_track) {
            widget_track->set_marked_for_deletion(true);
            widget_track = nullptr;
          }
          if (widget_header) {
            widget_header->set_marked_for_deletion(true);
            widget_header = nullptr;
          }
        }
        Widget* widget() {
          if (widget_track) {
            return widget_track;
          } else if (widget_header) {
            return widget_header;
          } else {
            return nullptr;
          }
        }

        i32 height() const {
          static const i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
          static const i32 header_height = theme::get_prop("tracklist_playlist_header_height").as_i32();
          if (type == ItemType::TRACK) {
            return track_height;
          } else if (type == ItemType::HEADER) {
            return header_height;
          }
          return 0;
        }
    };

    enum class InsertCursorPos : u8 { BELOW, ABOVE };
    PanelTracks(UI& ui_);
    ~PanelTracks();

    void event(Input::InputEventMouseScroll&) override;
    void event(Input::InputEventMouseButton&) override;
    void update() override;
    void draw() override;

    void recreate(std::optional<size_t> collection_id_);
    void recreate(std::span<const db::track_info> tracks);
    void insert_track(size_t, db::track_info);
    void insert_track(db::track_info);
    void set_track(size_t, db::track_info);
    void remove_item(size_t index);

    void clear();

    void scroll_to_playlist(size_t playlist_id);
    void scroll_to_track(size_t playlist_id, size_t track_id);
    void set_scroll_px(float px);
    float get_scroll_px() const;

    std::span<const Item> get_items() const { return items; }
    const PanelTracksSelection& selection() const { return m_selection; }
    void clear_selection() {
      m_selection.clear();
      selection_modified = true;
    }

    void set_insert_cursor_track_info(std::optional<db::track_info> value) { insert_cursor_track_info = value; }
    void set_insert_cursor_pos(InsertCursorPos value) { insert_cursor_pos = value; }
    void set_track_highlight_mode(WidgetTrack::TrackHighlightMode value) { track_highlight_mode = value; }

  protected:
    WidgetTrack::TrackHighlightMode track_highlight_mode = WidgetTrack::TrackHighlightMode::TRACK_INFO;
    PanelTracksSelection m_selection;

  public:
    std::function<void(db::track_info, WidgetTrack*)> on_track_lmb{};
    std::function<void(db::track_info, WidgetTrack*)> on_track_rmb{};
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
    Sprite* insert_cursor{};
    InsertCursorPos insert_cursor_pos = InsertCursorPos::BELOW;
    std::optional<db::track_info> insert_cursor_track_info{};
    ToolTip* button_play_tooltip{};
    ToolTip* button_play_next_tooltip{};
    ToolTip* button_sort_tooltip{};
    ToolTip* button_more_tooltip{};

    void create_item_widget_if_null(Item&);
    std::vector<Item> items;
    Widget* items_container{};
};
