#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "common/search_utils.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/types.hpp"
#include "ui/panel_playlists.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/widget_track.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/checkbox.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

class PopupSearch final : public Popup {
    using Widget::event;

  public:
    PopupSearch(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    void update() override;

    void event(zincgui::Input::InputEventKey& e) override;

    void event(zincgui::Input::InputEventMouseScroll& e) override;

    void update_search_results();

    void relayout();

    [[nodiscard]] db::collection_id_t collection_id() const noexcept { return m_collection_id; }
    PopupSearch& collection_id(db::collection_id_t id) noexcept {
      m_collection_id = id;
      return *this;
    }

    PopupSearch& on_playlist_lmb(std::function<void(size_t playlist_id, Widget*)> fn) {
      m_on_playlist_lmb = std::move(fn);
      return *this;
    }
    PopupSearch& on_playlist_rmb(std::function<void(size_t playlist_id, Widget*)> fn) {
      m_on_playlist_rmb = std::move(fn);
      return *this;
    }
    PopupSearch& on_track_lmb(std::function<void(db::track_info, Widget*)> fn) {
      m_on_track_lmb = std::move(fn);
      return *this;
    }
    PopupSearch& on_track_rmb(std::function<void(db::track_info, Widget*)> fn) {
      m_on_track_rmb = std::move(fn);
      return *this;
    }
    PopupSearch& on_closed(std::function<void()> fn) {
      m_on_closed = std::move(fn);
      return *this;
    }
    [[nodiscard]] const std::function<void()>& on_closed() const noexcept { return m_on_closed; }

    [[nodiscard]] zincgui::TextInput* search_bar() noexcept { return m_search_bar; }
    [[nodiscard]] const zincgui::TextInput* search_bar() const noexcept { return m_search_bar; }

  private:
    zincgui::TextInput* m_search_bar{};
    Widget* m_search_results{};
    Widget* m_scrollable_content{};
    zincgui::ScrollBar* m_scrollbar{};
    PanelPlaylists* m_playlists_container{};
    Widget* m_tracks_container{};
    Widget* m_buttons{};
    zincgui::Button* m_button_close{};
    zincgui::Checkbox* m_checkbox_search_all_collections{};
    zincgui::Label* m_label_playlists{};
    zincgui::Label* m_label_tracks{};
    zincgui::Label* m_label_no_results{};
    bool m_checkbox_search_all_collections_state_old = true;

    size_t m_collection_id{};
    std::string m_search_text;
    std::vector<size_t> m_playlist_ids;
    std::vector<db::track_info> m_found_tracks;
    double m_scroll_px{};
    double m_target_scroll_px{};
    i32 m_width_old = -1;
    i32 m_height_old = -1;
    i32 m_resize_hack_fixme = 0;

    std::function<void(size_t playlist_id, Widget*)> m_on_playlist_lmb;
    std::function<void(size_t playlist_id, Widget*)> m_on_playlist_rmb;
    std::function<void(db::track_info, Widget*)> m_on_track_lmb;
    std::function<void(db::track_info, Widget*)> m_on_track_rmb;
    std::function<void()> m_on_closed;

    static constexpr size_t MAX_PLAYLISTS = 15;
    static constexpr size_t MAX_TRACKS = 40;
};
