#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/widget_playlist_header.hpp"
#include "ui/widget_queue_playlist_collection.hpp"
#include "ui/widget_track.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "zincbox.hpp"

namespace zincgui {
  class Sprite;
  class ToolTip;
  class Root;
  class Widget;
  class ScrollBar;
} // namespace zincgui

class PanelTracklistSelection final {
  public:
    bool has(size_t s) const { return m_selection_set.contains(s); }
    bool insert(size_t s) {

      if (m_selection_set.emplace(s).second) {
        m_selection_vector.emplace_back(s);
        return true;
      }
      return false;
    }
    void erase(size_t s) {
      if (m_selection_set.erase(s)) {
        std::erase_if(m_selection_vector, [&s](size_t s_) -> bool { return s == s_; });
      }
    }

    bool empty() const { return m_selection_vector.empty(); }

    bool clear() {
      ensure(m_selection_set.empty() == m_selection_vector.empty());
      if (m_selection_vector.empty()) { return false; }
      m_selection_set.clear();
      m_selection_vector.clear();
      return true;
    }

    std::optional<size_t> back() const {
      if (m_selection_vector.empty()) { return std::nullopt; }
      return m_selection_vector.back();
    }

    const std::vector<size_t>& get() const { return m_selection_vector; }

    size_t size() const { return m_selection_vector.size(); }

  private:
    std::unordered_set<size_t> m_selection_set;
    std::vector<size_t> m_selection_vector;
};

class PanelTracklist final : public zincgui::ColorRect {
  public:
    using ColorRect::event;

    struct TrackInfoHash {
        size_t operator()(const db::track_info& t) const {
          return std::hash<size_t>()(t.collection_id) ^ (std::hash<size_t>()(t.playlist_id) << 1) ^
                 (std::hash<size_t>()(t.track_id) << 2) ^ (std::hash<size_t>()(t.index) << 3);
        }
    };

    struct ItemTrack {
        static constexpr bool is_selectable = true;
        db::track_info track_info{};
        WidgetTrack* widget = nullptr;
        size_t item_index = 0;
        i32 height() const { return theme::config().panel_tracklist.track_height * zincbox::ui_scale(); }
    };

    struct ItemQueueTrack {
        static constexpr bool is_selectable = true;
        size_t queue_index = 0;
        WidgetTrack* widget = nullptr;
        size_t item_index = 0;
        i32 height() const { return theme::config().panel_tracklist.track_height * zincbox::ui_scale(); }
    };

    struct ItemQueuePlaylist {
        static constexpr bool is_selectable = true;
        size_t queue_index = 0;
        WidgetQueuePlaylistCollection* widget = nullptr;
        size_t item_index = 0;
        i32 height() const { return theme::config().panel_queue.playlist_height * zincbox::ui_scale(); }
    };

    struct ItemQueueCollection {
        static constexpr bool is_selectable = true;
        size_t queue_index = 0;
        WidgetQueuePlaylistCollection* widget = nullptr;
        size_t item_index = 0;
        i32 height() const { return theme::config().panel_queue.collection_height * zincbox::ui_scale(); }
    };

    struct ItemPlaylistHeader {
        static constexpr bool is_selectable = false;
        db::collection_id_t collection_id = db::INVALID_ID;
        db::playlist_id_t playlist_id = db::INVALID_ID;
        WidgetPlaylistHeader* widget = nullptr;
        size_t item_index = 0;
        i32 height() const {
          return (theme::config().panel_tracklist.header_height + 2 * theme::config().panel_tracklist.header_spacing) *
                 zincbox::ui_scale();
        }
    };

    using Item = std::variant<ItemTrack, ItemQueueTrack, ItemQueuePlaylist, ItemQueueCollection, ItemPlaylistHeader>;

    enum class InsertCursorPos : u8 { BELOW, ABOVE };
    PanelTracklist(const PanelTracklist&) = delete;
    PanelTracklist(PanelTracklist&&) = delete;
    PanelTracklist& operator=(const PanelTracklist&) = delete;
    PanelTracklist& operator=(PanelTracklist&&) = delete;
    PanelTracklist(zincgui::Root& ui_);
    ~PanelTracklist() override;

    void event(zincgui::Input::InputEventMouseScroll&) override;
    void event(zincgui::Input::InputEventMouseButton&) override;
    void event(zincgui::Input::InputEventKey&) override;
    void input() override;
    void update() override;

    void show();
    void hide();
    void recreate(std::optional<size_t> collection_id_);
    void recreate(std::span<const db::track_info> tracks);
    void insert_track(size_t, db::track_info);
    void insert_track(db::track_info);
    void recreate(std::span<const Item> new_items);
    void insert_item(const Item& item);
    void insert_item(size_t index, const Item& item);
    void set_item(size_t index, const Item& item);
    void insert_to_selection_index(size_t index);
    void set_track(size_t, db::track_info);
    void remove_item(size_t index);

    void clear();

    void scroll_to_playlist(size_t playlist_id, bool immediate = false);
    void scroll_to_track(size_t playlist_id, size_t track_id, bool immediate = false);
    void set_scroll_px(float px, bool immediate = false);
    float get_scroll_px() const;

    const Item& get_item(size_t i) const {
      ensure(i < m_items.size());
      return m_items[i];
    }
    std::span<const Item> get_items() const { return m_items; }
    const PanelTracklistSelection& selection() const { return m_selection; }
    void clear_selection() {
      m_selection.clear();
      m_selection_modified = true;
    }

    std::optional<size_t> find_item_track(db::track_info ti) const;

    void insert_to_selection_track(db::track_info track_info) {
      auto idx = find_item_track(track_info);
      if (idx) {
        if (m_selection.insert(*idx)) { m_selection_modified = true; }
      }
    }

    void set_insert_cursor_index(std::optional<size_t> value) { m_insert_cursor_index = value; }
    void set_insert_cursor_pos(InsertCursorPos value) { m_insert_cursor_pos = value; }
    void set_track_highlight_mode(WidgetTrack::TrackHighlightMode value) { m_track_highlight_mode = value; }
    void set_is_dragged(bool value) { m_is_dragged = value; }

  private:
    WidgetTrack::TrackHighlightMode m_track_highlight_mode = WidgetTrack::TrackHighlightMode::TRACK_INFO;
    PanelTracklistSelection m_selection;

  public:
    std::function<void(size_t item_index)> on_item_lmb{};
    std::function<void(size_t item_index)> on_item_rmb{};
    std::function<void(size_t item_index)> on_selection_rmb{};

    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_more_options_invoked{};
    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> on_playlist_sort_button_pressed{};

  private:
    std::optional<size_t> m_collection_id;
    double m_scroll_px{};
    double m_target_scroll_px{};
    double m_old_scroll_px{};
    vec2i m_old_size{};
    i32 m_max_scroll_px{};
    float m_scroll_edge_factor = 0.0f;
    bool m_is_dragged = false;
    bool m_just_recreated = false;
    bool m_selection_modified = false;
    zincgui::ScrollBar* m_scrollbar{};
    zincgui::ColorRect* m_insert_cursor{};
    InsertCursorPos m_insert_cursor_pos = InsertCursorPos::BELOW;
    std::optional<size_t> m_insert_cursor_index;
    zincgui::ToolTip* m_button_play_tooltip{};
    zincgui::ToolTip* m_button_play_next_tooltip{};
    zincgui::ToolTip* m_button_sort_tooltip{};
    zincgui::ToolTip* m_button_more_tooltip{};

    static i32 get_item_height(const Item& item);
    static bool is_item_selectable(const Item& item);
    static zincgui::Widget* get_item_widget(Item& item);
    static void delete_item_widget(Item& item);
    static size_t get_item_index(const Item& item);
    static void set_item_index(Item& item, size_t index);
    static void set_item_selected(Item& item, bool selected);

    void create_item_widget_if_null(Item&);
    void rebuild_lookup();

    std::vector<Item> m_items;
    std::vector<size_t> m_header_indices; // used for tooltip handling but FIXME remove this and handle tooltips
                                                  // in each header individually when we have a TooltipManager
    std::unordered_map<db::track_info, size_t, TrackInfoHash> m_track_lookup;
    Widget* m_items_container{};
};
