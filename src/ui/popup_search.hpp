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
    PopupSearch(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {
      m_playlist_ids.reserve(MAX_PLAYLISTS);
      m_found_tracks.reserve(MAX_TRACKS);

      layout("ttb expand fill m:8 s:8");

      m_search_bar = &add_child<zincgui::TextInput>();
      m_search_bar->on_text_changed([this]() -> void { update_search_results(); });
      ui().set_focused_widget(m_search_bar);

      m_search_results = &add_child<zincgui::ColorRect>(theme::config().panel_playlists.color);
      m_search_results->layout("ltr fill expand").clip_children(true);

      m_scrollable_content = &m_search_results->add_child<Widget>();
      m_scrollable_content->clip_children(true);

      m_scrollbar = &m_search_results->add_child<zincgui::ScrollBar>();
      m_scrollbar->anchor(zincgui::Anchor::TOP_RIGHT)
        .parent_anchor(zincgui::Anchor::TOP_RIGHT)
        .orientation(zincgui::SliderOrientation::VERTICAL);
      m_scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

      m_label_playlists = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.results_albums_playlists"));
      m_label_playlists->text_color(theme::config().text_color_muted)
        .resize_to_text_extents(false)
        .x(8)
        .height(32)
        .is_drawn(false);

      m_playlists_container = &m_scrollable_content->add_child<PanelPlaylists>();
      m_playlists_container->props().playlist_ids = {};
      m_playlists_container->props().panel_search_visible = false;
      m_playlists_container->props().is_scrollable = false;

      m_label_no_results = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.no_results"));
      m_label_no_results->text_color(theme::config().text_color_muted)
        .resize_to_text_extents(false)
        .label_anchor(zincgui::Anchor::CENTER)
        .parent_anchor(zincgui::Anchor::CENTER)
        .anchor(zincgui::Anchor::CENTER)
        .is_drawn(false);

      m_label_tracks = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.results_tracks"));
      m_label_tracks->text_color(theme::config().text_color_muted)
        .resize_to_text_extents(false)
        .x(8)
        .height(32)
        .is_drawn(false);

      m_tracks_container = &m_scrollable_content->add_child<Widget>();
      m_tracks_container->layout("ttb fill fit expand");

      m_buttons = &add_child<Widget>();
      m_buttons->layout("ltr expand fill m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);
      m_button_close = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.close"));
      m_button_close->on_press([this]() {
        if (m_on_closed) { m_on_closed(); }
        close();
      });
      m_checkbox_search_all_collections = &m_buttons->add_child<zincgui::Checkbox>(tr::get("search.all_collections"));
      m_checkbox_search_all_collections->checked(true);
    }

    void update() override {
      width(max_content_width()).height(max_content_height());
      Popup::update();

      if (m_checkbox_search_all_collections_state_old != m_checkbox_search_all_collections->checked()) {
        m_checkbox_search_all_collections_state_old = m_checkbox_search_all_collections->checked();
        m_search_text.clear();
        update_search_results();
      }

      if (std::abs(m_scroll_px - m_target_scroll_px) > 0.5 || m_width_old != width() || m_height_old != height()) {
        m_width_old = width();
        m_height_old = height();

        m_resize_hack_fixme = 2;
      }

      if (m_resize_hack_fixme-- > 0) {
        relayout();
        mark_dirty();
        mark_layout_dirty();
      }

      if (std::abs(m_scroll_px - m_target_scroll_px) > 0.5) {
        double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
        m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);
      }
    }

    void event(zincgui::Input::InputEventKey& e) override {
      if (e.key == zincgui::Input::Key::KEY_ESCAPE && e.action == zincgui::Input::KeyAction::RELEASE) {
        if (m_on_closed) { m_on_closed(); }
        close();
        e.handled = true;
      }
    }

    void event(zincgui::Input::InputEventMouseScroll& e) override {
      if (is_mouse_hovering()) {
        m_scrollbar->scroll(e.offset.y);
        e.handled = true;
      }
    }

    void update_search_results() {
      auto new_search_text = sanitize_query(m_search_bar->label().text());
      if (m_search_text == new_search_text) { return; }

      m_search_text = new_search_text;
      m_playlist_ids.clear();
      m_found_tracks.clear();

      for (auto&& t : m_tracks_container->children()) {
        t->is_marked_for_deletion(true);
      }

      if (new_search_text.empty()) {
        m_playlists_container->props().playlist_ids = {};
        m_playlists_container->recreate();
        m_scrollbar->scroll_offset(0);
        m_scroll_px = 0.0;
        m_target_scroll_px = 0.0;
        relayout();
        return;
      }

      if (m_checkbox_search_all_collections->checked()) {
        auto res = db::search_playlists(m_search_text, MAX_PLAYLISTS);
        for (auto& r : res) {
          m_playlist_ids.emplace_back(r.playlist_id);
        }
        m_found_tracks = db::search_tracks(m_search_text, MAX_TRACKS);
      } else {
        auto res = db::search_playlists(m_search_text, m_collection_id, MAX_PLAYLISTS);
        for (auto& r : res) {
          m_playlist_ids.emplace_back(r.playlist_id);
        }
        m_found_tracks = db::search_tracks(m_search_text, m_collection_id, MAX_TRACKS);
      }

      m_playlists_container->props().playlist_ids = m_playlist_ids;
      m_playlists_container
        ->on_playlist_lmb([this](size_t playlist_id, Widget* w) -> void {
          if (m_on_playlist_lmb) { m_on_playlist_lmb(playlist_id, w); }
        })
        .on_playlist_rmb([this](size_t playlist_id, Widget* w) -> void {
          if (m_on_playlist_rmb) { m_on_playlist_rmb(playlist_id, w); }
        });

      i32 i = 1;
      i32 track_height = theme::config().panel_tracklist.track_height * zincbox::ui_scale();
      for (db::track_info& ti : m_found_tracks) {
        auto* w = &m_tracks_container->add_child<WidgetTrack>();
        w->item_index(i - 1).track_id(ti.track_id).track_number(i).fixed_height(track_height);
        w->on_press([this, ti, w]() {
          if (m_on_track_lmb) { m_on_track_lmb(ti, w); }
        });
        w->on_press_rmb([this, ti, w]() {
          if (m_on_track_rmb) { m_on_track_rmb(ti, w); }
        });
        i += 1;
      }

      m_scrollbar->scroll_offset(0);
      m_scroll_px = 0.0;
      m_target_scroll_px = 0.0;

      m_playlists_container->recreate();
      relayout();
    }

    void relayout() {
      m_label_playlists->is_drawn(!m_playlist_ids.empty());
      m_label_tracks->is_drawn(!m_found_tracks.empty());
      if (m_playlist_ids.empty() && m_found_tracks.empty()) {
        bool no_search_text = sanitize_query(m_search_bar->label().text()).empty();
        m_label_no_results->is_drawn(!no_search_text);
      } else {
        m_label_no_results->is_drawn(false);
      }

      i32 results_w = m_search_results->width();
      i32 page_size = m_search_results->height();
      i32 track_height = theme::config().panel_tracklist.track_height * zincbox::ui_scale();
      i32 track_total_height = static_cast<i32>(m_found_tracks.size()) * track_height;

      auto compute_heights_for_width = [&](i32 w) -> i32 {
        m_playlists_container->width(w);
        if (!m_playlist_ids.empty()) {
          m_playlists_container->reflow();
          m_playlists_container->height(m_playlists_container->content_size().y);
        } else {
          m_playlists_container->height(0);
        }

        i32 total = m_playlists_container->height() + track_total_height;
        if (m_label_playlists->is_drawn()) { total += m_label_playlists->height(); }
        if (m_label_tracks->is_drawn()) { total += m_label_tracks->height(); }
        return total;
      };

      i32 content_sz = compute_heights_for_width(results_w);
      bool needs_scrollbar = content_sz > page_size;
      i32 target_w = needs_scrollbar ? (results_w - m_scrollbar->width()) : results_w;

      if (needs_scrollbar) { content_sz = compute_heights_for_width(target_w); }

      m_scrollable_content->width(target_w).height(page_size);
      m_tracks_container->width(target_w).height(track_total_height);

      m_scrollbar->content_size(content_sz).page_size(page_size).height(page_size).is_drawn(needs_scrollbar);

      i32 y_sum = 0;
      for (Widget* w :
           std::array<Widget*, 4>{m_label_playlists, m_playlists_container, m_label_tracks, m_tracks_container}) {
        if (!w->is_drawn()) { continue; }
        w->y(-std::floor(m_scroll_px) + y_sum);
        y_sum += w->height();
      }
    }

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
