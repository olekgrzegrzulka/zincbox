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
#include "ui/tr.hpp"
#include "ui/widget_track.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/checkbox.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
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

      set_layout("ttb expand fill m:8 s:8");

      m_search_bar = &add_child<zincgui::TextInput>();
      m_search_bar->set_on_text_changed([this]() -> void { update_search_results(); });
      m_search_bar->set_fixed_height((8 * zincbox::ui_scale()) + zincbox::font_size());
      m_search_bar->set_focused(true);

      m_search_results = &add_child<zincgui::ColorRect>(theme::config().panel_playlists.color);
      m_search_results->set_layout("ltr fill expand");
      m_search_results->set_clip_children(true);

      m_scrollable_content = &m_search_results->add_child<Widget>();
      m_scrollable_content->set_clip_children(true);

      m_scrollbar = &m_search_results->add_child<ZincboxScrollbar>();
      m_scrollbar->set_anchor(zincgui::Anchor::TOP_RIGHT);
      m_scrollbar->set_parent_anchor(zincgui::Anchor::TOP_RIGHT);
      m_scrollbar->set_orientation(zincgui::SliderOrientation::VERTICAL);
      m_scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

      m_label_playlists = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.results_albums_playlists"));
      m_label_playlists->set_text_color(theme::config().text_color_muted);
      m_label_playlists->set_resize_to_text_extents(false);
      m_label_playlists->set_x(8);
      m_label_playlists->set_height(32);
      m_label_playlists->set_is_drawn(false);

      m_playlists_container = &m_scrollable_content->add_child<PanelPlaylists>();
      m_playlists_container->props.playlist_ids = {};
      m_playlists_container->props.panel_search_visible = false;
      m_playlists_container->props.is_scrollable = false;

      m_label_no_results = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.no_results"));
      m_label_no_results->set_text_color(theme::config().text_color_muted);
      m_label_no_results->set_resize_to_text_extents(false);
      m_label_no_results->set_label_anchor(zincgui::Anchor::CENTER);
      m_label_no_results->set_parent_anchor(zincgui::Anchor::CENTER);
      m_label_no_results->set_anchor(zincgui::Anchor::CENTER);
      m_label_no_results->set_is_drawn(false);

      m_label_tracks = &m_scrollable_content->add_child<zincgui::Label>(tr::get("search.results_tracks"));
      m_label_tracks->set_text_color(theme::config().text_color_muted);
      m_label_tracks->set_resize_to_text_extents(false);
      m_label_tracks->set_x(8);
      m_label_tracks->set_height(32);
      m_label_tracks->set_is_drawn(false);

      m_tracks_container = &m_scrollable_content->add_child<Widget>();
      m_tracks_container->set_layout("ttb fill fit expand");

      m_buttons = &add_child<Widget>();
      m_buttons->set_layout("ltr expand fill m:0 s:8");
      m_buttons->set_fixed_height(32 * zincbox::ui_scale());
      m_button_close = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.close"));
      m_button_close->on_press([this]() {
        if (on_closed) { on_closed(); }
        close();
      });
      m_checkbox_search_all_collections = &m_buttons->add_child<zincgui::Checkbox>(tr::get("search.all_collections"));
      m_checkbox_search_all_collections->set_checked(true);
    }

    void update_search_results() {
      auto new_search_text = sanitize_query(m_search_bar->label.get_text());
      if (m_search_text == new_search_text) { return; }

      m_search_text = new_search_text;
      m_playlist_ids.clear();
      m_found_tracks.clear();

      for (auto&& t : m_tracks_container->get_children()) {
        t->set_marked_for_deletion(true);
      }

      if (new_search_text.empty()) {
        m_playlists_container->props.playlist_ids = {};
        m_playlists_container->recreate();
        m_scrollbar->set_scroll_offset(0);
        m_scroll_px = 0.0;
        m_target_scroll_px = 0.0;
        relayout();
        return;
      }

      if (m_checkbox_search_all_collections->is_checked()) {
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

      m_playlists_container->props.playlist_ids = m_playlist_ids;
      m_playlists_container->on_playlist_lmb = [this](size_t playlist_id, Widget* w) -> void {
        if (on_playlist_lmb) { on_playlist_lmb(playlist_id, w); }
      };
      m_playlists_container->on_playlist_rmb = [this](size_t playlist_id, Widget* w) -> void {
        if (on_playlist_rmb) { on_playlist_rmb(playlist_id, w); }
      };

      i32 i = 1;
      i32 track_height = theme::config().panel_tracklist.track_height * zincbox::ui_scale();
      for (db::track_info& ti : m_found_tracks) {
        auto* w = &m_tracks_container->add_child<WidgetTrack>();
        w->item_index(i - 1);
        w->track_id(ti.track_id).track_number(i);
        w->set_fixed_height(track_height);
        w->on_press([this, ti, w]() {
          if (on_track_lmb) { on_track_lmb(ti, w); }
        });
        w->on_press_rmb([this, ti, w]() {
          if (on_track_rmb) { on_track_rmb(ti, w); }
        });
        i += 1;
      }

      m_scrollbar->set_scroll_offset(0);
      m_scroll_px = 0.0;
      m_target_scroll_px = 0.0;

      m_playlists_container->recreate();
      relayout();
    }

    void relayout() {
      m_label_playlists->set_is_drawn(!m_playlist_ids.empty());
      m_label_tracks->set_is_drawn(!m_found_tracks.empty());
      if (m_playlist_ids.empty() && m_found_tracks.empty()) {
        bool no_search_text = sanitize_query(m_search_bar->label.get_text()).empty();
        m_label_no_results->set_is_drawn(!no_search_text);
      } else {
        m_label_no_results->set_is_drawn(false);
      }

      i32 results_w = m_search_results->get_width();
      i32 page_size = m_search_results->get_height();
      i32 track_height = theme::config().panel_tracklist.track_height * zincbox::ui_scale();
      i32 track_total_height = static_cast<i32>(m_found_tracks.size()) * track_height;

      auto compute_heights_for_width = [&](i32 w) -> i32 {
        m_playlists_container->set_width(w);
        if (!m_playlist_ids.empty()) {
          m_playlists_container->reflow();
          m_playlists_container->set_height(m_playlists_container->get_content_size().y);
        } else {
          m_playlists_container->set_height(0);
        }

        i32 total = m_playlists_container->get_height() + track_total_height;
        if (m_label_playlists->get_is_drawn()) { total += m_label_playlists->get_height(); }
        if (m_label_tracks->get_is_drawn()) { total += m_label_tracks->get_height(); }
        return total;
      };

      i32 content_size = compute_heights_for_width(results_w);
      bool needs_scrollbar = content_size > page_size;
      i32 target_w = needs_scrollbar ? (results_w - m_scrollbar->get_width()) : results_w;

      if (needs_scrollbar) { content_size = compute_heights_for_width(target_w); }

      m_scrollable_content->set_width(target_w);
      m_scrollable_content->set_height(page_size);
      m_tracks_container->set_width(target_w);
      m_tracks_container->set_height(track_total_height);

      m_scrollbar->set_content_size(content_size);
      m_scrollbar->set_page_size(page_size);
      m_scrollbar->set_height(page_size);
      m_scrollbar->set_is_drawn(needs_scrollbar);

      i32 y_sum = 0;
      for (Widget* w :
           std::array<Widget*, 4>{m_label_playlists, m_playlists_container, m_label_tracks, m_tracks_container}) {
        if (!w->get_is_drawn()) { continue; }
        w->set_y(-std::floor(m_scroll_px) + y_sum);
        y_sum += w->get_height();
      }
    }

    void update() override {
      set_width(get_max_content_width());
      set_height(get_max_content_height());
      Popup::update();

      if (m_checkbox_search_all_collections_state_old != m_checkbox_search_all_collections->is_checked()) {
        m_checkbox_search_all_collections_state_old = m_checkbox_search_all_collections->is_checked();
        m_search_text.clear();
        update_search_results();
      }

      if (m_scroll_px != m_target_scroll_px) {
        double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
        m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);
      }

      relayout();
    }

    void event(zincgui::Input::InputEventKey& e) override {
      if (e.key == zincgui::Input::Key::KEY_ESCAPE && e.action == zincgui::Input::KeyAction::RELEASE) {
        if (on_closed) { on_closed(); }
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

    void set_collection_id(db::collection_id_t collection_id_) { m_collection_id = collection_id_; }

  public:
    std::function<void(size_t playlist_id, Widget*)> on_playlist_lmb{};
    std::function<void(size_t playlist_id, Widget*)> on_playlist_rmb{};
    std::function<void(db::track_info, Widget*)> on_track_lmb{};
    std::function<void(db::track_info, Widget*)> on_track_rmb{};
    std::function<void()> on_closed{};

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

    static constexpr size_t MAX_PLAYLISTS = 15;
    static constexpr size_t MAX_TRACKS = 40;
};
