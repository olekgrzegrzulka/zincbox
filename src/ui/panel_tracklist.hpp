#pragma once
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
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

    const std::vector<size_t>& selection_vector() const noexcept { return m_selection_vector; }
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

    void input() override;
    void update() override;
    void event(zincgui::Input::InputEventMouseScroll& e) override;
    void event(zincgui::Input::InputEventMouseButton& e) override;
    void event(zincgui::Input::InputEventKey& e) override;

    void show();
    void hide();
    void recreate(std::optional<size_t> collection_id_);
    void recreate(std::span<const db::track_info> tracks);
    void recreate(std::span<const Item> new_items);
    void insert_track(size_t index, db::track_info ti);
    void insert_track(db::track_info ti);
    void insert_item(const Item& item);
    void insert_item(size_t index, const Item& item);
    void set_item(size_t index, const Item& item);
    void set_track(size_t index, db::track_info ti);
    void remove_item(size_t index);
    void clear();
    void clear_selection();
    void insert_to_selection_index(size_t index);
    void insert_to_selection_track(db::track_info track_info);
    void scroll_to_playlist(size_t playlist_id, bool immediate = false);
    void scroll_to_track(size_t playlist_id, size_t track_id, bool immediate = false);
    [[nodiscard]] std::optional<size_t> find_item_track(db::track_info ti) const;

    [[nodiscard]] float scroll_px() const;
    PanelTracklist& scroll_px(float px, bool immediate = false);

    [[nodiscard]] const Item& item(size_t i) const {
      ensure(i < m_items.size());
      return m_items[i];
    }

    [[nodiscard]] std::span<const Item> items() const noexcept { return m_items; }

    [[nodiscard]] const PanelTracklistSelection& selection() const noexcept { return m_selection; }

    PanelTracklist& insert_cursor_index(std::optional<size_t> value) noexcept {
      m_insert_cursor_index = value;
      return *this;
    }

    PanelTracklist& insert_cursor_pos(InsertCursorPos value) noexcept {
      m_insert_cursor_pos = value;
      return *this;
    }

    PanelTracklist& track_highlight_mode(WidgetTrack::TrackHighlightMode value) noexcept {
      m_track_highlight_mode = value;
      return *this;
    }

    PanelTracklist& is_dragged(bool value) noexcept {
      m_is_dragged = value;
      return *this;
    }

    PanelTracklist& on_item_lmb(std::function<void(size_t item_index)> cb) {
      m_on_item_lmb = std::move(cb);
      return *this;
    }
    PanelTracklist& on_item_rmb(std::function<void(size_t item_index)> cb) {
      m_on_item_rmb = std::move(cb);
      return *this;
    }
    PanelTracklist& on_selection_rmb(std::function<void(size_t item_index)> cb) {
      m_on_selection_rmb = std::move(cb);
      return *this;
    }
    PanelTracklist& on_playlist_more_options_invoked(std::function<void(size_t col_id, size_t pl_id, Widget*)> cb) {
      m_on_playlist_more_options_invoked = std::move(cb);
      return *this;
    }
    PanelTracklist& on_playlist_sort_button_pressed(std::function<void(size_t col_id, size_t pl_id, Widget*)> cb) {
      m_on_playlist_sort_button_pressed = std::move(cb);
      return *this;
    }

  private:
    static i32 get_item_height(const Item& item);
    static bool is_item_selectable(const Item& item);
    static zincgui::Widget* get_item_widget(Item& item);
    static void delete_item_widget(Item& item);
    static size_t get_item_index(const Item& item);
    static void set_item_index(Item& item, size_t index);
    static void set_item_selected(Item& item, bool selected);

    void create_item_widget_if_null(Item& item);
    void rebuild_lookup();

    WidgetTrack::TrackHighlightMode m_track_highlight_mode = WidgetTrack::TrackHighlightMode::TRACK_INFO;
    PanelTracklistSelection m_selection;

    std::function<void(size_t item_index)> m_on_item_lmb{};
    std::function<void(size_t item_index)> m_on_item_rmb{};
    std::function<void(size_t item_index)> m_on_selection_rmb{};
    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> m_on_playlist_more_options_invoked{};
    std::function<void(size_t collection_id, size_t playlist_id, Widget*)> m_on_playlist_sort_button_pressed{};

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

    std::vector<Item> m_items;
    std::unordered_map<db::track_info, size_t, TrackInfoHash> m_track_lookup;
    Widget* m_items_container{};
};
