#include <functional>
#include "common/input.hpp"
#include "common/search_utils.hpp"
#include "core/musicdb/musicdb.hpp"
#include "tr.hpp"
#include "ui/panel_albums.hpp"
#include "ui/popup_controller.hpp"
#include "ui/theme.hpp"
#include "ui/widget_track.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/checkbox.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/widget.hpp"

class PopupSearch : public Popup {
    using Widget::event;

  public:
    PopupSearch(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {
      playlist_ids.reserve(MAX_PLAYLISTS);
      found_tracks.reserve(MAX_TRACKS);

      get_layout().enabled = false;

      search_bar = &add_child<TextInput>();
      search_bar->set_on_text_changed([this]() { update_search_results(); });
      search_bar->set_height(22);
      search_bar->set_min_height(22);
      search_bar->set_max_height(22);
      search_bar->set_anchor(Anchor::TOP);
      search_bar->set_parent_anchor(Anchor::TOP);
      search_bar->set_pos(0, 8);
      search_bar->set_focused(true);

      search_results = &add_child<Sprite>("panel_albums");
      search_results->set_layout("ltr fill expand");
      search_results->set_clip_children(true);
      search_results->set_anchor(Anchor::TOP);
      search_results->set_parent_anchor(Anchor::TOP);
      search_results->set_pos(0, 8 + search_bar->get_height() + 8);

      scrollable_content = &search_results->add_child<Widget>();
      scrollable_content->set_clip_children(true);

      scrollbar = &search_results->add_child<ZincboxScrollbar>();
      scrollbar->set_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_parent_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_thumb_thickness(10);
      scrollbar->set_track_thickness(10);
      scrollbar->set_max_width(10);
      scrollbar->set_orientation(SliderOrientation::VERTICAL);
      scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });

      label_playlists = &scrollable_content->add_child<Label>(tr::get("search.results_albums_playlists"));
      label_playlists->set_text_color(theme::get_prop("text_color_muted").as_rgba());
      label_playlists->set_resize_to_text_extents(false);
      label_playlists->set_x(8);
      label_playlists->set_height(32);
      label_playlists->set_is_drawn(false);

      playlists_container = &scrollable_content->add_child<PanelAlbums>();
      playlists_container->props.playlist_ids = {};
      playlists_container->props.panel_search_visible = false;
      playlists_container->props.is_scrollable = false;
      playlists_container->props.cover_width = 48;
      playlists_container->props.cover_min_horizontal_spacing = 12;
      playlists_container->props.cover_min_vertical_spacing = 44;

      label_no_results = &scrollable_content->add_child<Label>(tr::get("search.no_results"));
      label_no_results->set_text_color(theme::get_prop("text_color_muted").as_rgba());
      label_no_results->set_resize_to_text_extents(false);
      label_no_results->set_label_anchor(Anchor::CENTER);
      label_no_results->set_parent_anchor(Anchor::CENTER);
      label_no_results->set_anchor(Anchor::CENTER);
      label_no_results->set_is_drawn(false);

      label_tracks = &scrollable_content->add_child<Label>(tr::get("search.results_tracks"));
      label_tracks->set_text_color(theme::get_prop("text_color_muted").as_rgba());
      label_tracks->set_resize_to_text_extents(false);
      label_tracks->set_x(8);
      label_tracks->set_height(32);
      label_tracks->set_is_drawn(false);

      tracks_container = &scrollable_content->add_child<Widget>();
      tracks_container->set_layout("ttb fill fit expand");

      buttons = &add_child<Widget>();
      buttons->set_layout("ltr fill fit expand m:0 s:16");
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);
      buttons->set_anchor(Anchor::BOTTOM);
      buttons->set_parent_anchor(Anchor::BOTTOM);
      buttons->set_pos(8, -8);
      button_close = &buttons->add_child<Button>(tr::get("dialog.action.close"));
      button_close->on_press([this]() {
        if (on_closed) { on_closed(); }
        close();
      });
      checkbox_search_all_collections = &buttons->add_child<Checkbox>(tr::get("search.all_collections"));
    }

    void update_search_results() {
      auto new_search_text = sanitize_query(search_bar->label.get_text());
      if (search_text == new_search_text) { return; }

      search_text = new_search_text;
      playlist_ids.clear();
      found_tracks.clear();
      if (new_search_text.empty()) {
        playlists_container->props.playlist_ids = {};
        for (auto&& t : tracks_container->get_children()) {
          t->set_marked_for_deletion(true);
        }
        scrollbar->set_scroll_offset(0);
        scroll_px = 0.1;
        target_scroll_px = 0.0;
        return;
      }
      if (checkbox_search_all_collections->is_checked()) {
        auto res = db::search_playlists(search_text, MAX_PLAYLISTS);
        for (auto& r : res) {
          playlist_ids.emplace_back(r.playlist_id);
        }
        found_tracks = db::search_tracks(search_text, MAX_TRACKS);
      } else {
        auto res = db::search_playlists(search_text, collection_id, MAX_PLAYLISTS);
        for (auto& r : res) {
          playlist_ids.emplace_back(r.playlist_id);
        }
        found_tracks = db::search_tracks(search_text, collection_id, MAX_TRACKS);
      }

      playlists_container->props.playlist_ids = playlist_ids;
      playlists_container->recreate();
      playlists_container->on_playlist_lmb = [this](size_t playlist_id, Widget* w) -> void {
        if (on_playlist_lmb) { on_playlist_lmb(playlist_id, w); }
      };
      playlists_container->on_playlist_rmb = [this](size_t playlist_id, Widget* w) -> void {
        if (on_playlist_rmb) { on_playlist_rmb(playlist_id, w); }
      };

      for (auto&& t : tracks_container->get_children()) {
        t->set_marked_for_deletion(true);
      }

      i32 i = 1;
      i32 track_height = theme::get_prop("tracklist_track_height").as_i32(22);
      for (db::track_info& ti : found_tracks) {
        auto* w = &tracks_container->add_child<WidgetTrack>();
        w->track_id(ti.track_id).track_number(i);
        w->set_min_height(track_height);
        w->set_max_height(track_height);
        w->on_press([this, ti, w]() {
          if (on_track_lmb) { on_track_lmb(ti, w); }
        });
        w->on_press_rmb([this, ti, w]() {
          if (on_track_rmb) { on_track_rmb(ti, w); }
        });
        i += 1;
      }

      scrollbar->set_scroll_offset(0);
      scroll_px = 0.1;
      target_scroll_px = 0.0;

      update();
    }

    void update() override {
      set_width(std::clamp(ui.get_window_width() - 100, 300, 600));
      set_height(std::clamp(ui.get_window_height() - 100, 300, 500));

      if (checkbox_search_all_collections_state_old != checkbox_search_all_collections->is_checked()) {
        checkbox_search_all_collections_state_old = checkbox_search_all_collections->is_checked();
        search_text.clear();
        update_search_results();
      }

      if (scroll_px != target_scroll_px) {
        double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
        scroll_px = std::lerp(scroll_px, target_scroll_px, t);

        i32 y_sum = 0;
        for (Widget* w : std::array<Widget*, 4>{label_playlists, playlists_container, label_tracks, tracks_container}) {
          if (!w->get_is_drawn()) { continue; }
          w->set_y(-std::floor(scroll_px) + y_sum);
          y_sum += w->get_height();
        }

        ui.mark_dirty_recursive(scrollable_content);
      }

      search_bar->set_width(get_width() - 16);
      label_tracks->set_is_drawn(found_tracks.size() > 0);
      if (playlist_ids.size() == 0 && found_tracks.size() == 0) {
        bool no_search_text = sanitize_query(search_bar->label.get_text()).empty();
        label_no_results->set_is_drawn(!no_search_text);
      } else {
        label_no_results->set_is_drawn(false);
      }

      search_results->set_width(get_width() - 16);
      search_results->set_height(get_height() - (22 + 32 + 4 * 8));

      buttons->set_width(get_width() - 16);

      playlists_container->set_width(scrollable_content->get_width());
      if (playlist_ids.size() > 0) {
        playlists_container->set_height(playlists_container->get_content_size().y);
        label_playlists->set_is_drawn(true);
      } else {
        playlists_container->set_height(0);
        label_playlists->set_is_drawn(false);
      }

      static i32 track_height = theme::get_prop("tracklist_track_height").as_i32(22);
      i32 track_total_height = found_tracks.size() * track_height;
      i32 content_size = playlists_container->get_height() + track_total_height;
      if (label_playlists->get_is_drawn()) { content_size += label_playlists->get_height(); }
      if (label_tracks->get_is_drawn()) { content_size += label_tracks->get_height(); }

      i32 page_size = scrollable_content->get_height();
      scrollbar->set_content_size(content_size);
      scrollbar->set_page_size(page_size);

      scrollbar->update();
      search_results->update();
      tracks_container->set_width(scrollable_content->get_width());
      playlists_container->set_width(scrollable_content->get_width());

      Popup::update();
    }

    void event(Input::InputEventKey& e) override {
      if (e.key == Input::Key::KEY_ESCAPE && e.action == Input::KeyAction::RELEASE) {
        if (on_closed) { on_closed(); }
        close();
        e.handled = true;
      }
    }

    void event(Input::InputEventMouseScroll& e) override {
      if (is_mouse_hovering()) {
        scrollbar->scroll(e.offset.y);
        e.handled = true;
      }
    }

  public:
    std::function<void(size_t playlist_id, Widget*)> on_playlist_lmb{};
    std::function<void(size_t playlist_id, Widget*)> on_playlist_rmb{};
    std::function<void(db::track_info, Widget*)> on_track_lmb{};
    std::function<void(db::track_info, Widget*)> on_track_rmb{};
    std::function<void()> on_closed{};

  protected:
    TextInput* search_bar{};
    Widget* search_results{};
    Widget* scrollable_content{};
    ScrollBar* scrollbar{};
    PanelAlbums* playlists_container{};
    Widget* tracks_container{};
    Widget* buttons{};
    Button* button_close{};
    Checkbox* checkbox_search_all_collections{};
    Label* label_playlists{};
    Label* label_tracks{};
    Label* label_no_results{};
    bool checkbox_search_all_collections_state_old = true;

    size_t collection_id{};
    std::u32string search_text{};
    std::vector<size_t> playlist_ids;
    std::vector<db::track_info> found_tracks;
    double scroll_px{};
    double target_scroll_px{};

    static constexpr size_t MAX_PLAYLISTS = 15;
    static constexpr size_t MAX_TRACKS = 40;
};
