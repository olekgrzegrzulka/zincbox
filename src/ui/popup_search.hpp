#include "common/input.hpp"
#include "common/search_utils.hpp"
#include "core/musicdb//musicdb.hpp"
#include "ui/panel_albums.hpp"
#include "ui/panel_tracks_track.hpp"
#include "ui/popup_controller.hpp"
#include "ui/theme.hpp"
#include "ui_generic/checkbox.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/widget.hpp"

class PopupSearch : public Popup {
  public:
    PopupSearch(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_) : Popup(ui_, controller_, on_close_) {
      playlist_ids.reserve(MAX_PLAYLISTS);
      found_tracks.reserve(MAX_TRACKS);

      get_layout().enabled = false;

      search_bar = &add_child<TextInput>();
      search_bar->set_on_text_changed([this]() {
        update_search_results();
      });
      search_bar->set_height(22);
      search_bar->set_min_height(22);
      search_bar->set_max_height(22);
      search_bar->set_anchor(Anchor::TOP);
      search_bar->set_parent_anchor(Anchor::TOP);
      search_bar->set_pos(0, 8);

      search_results = &add_child<Sprite>("panel_albums");
      search_results->set_layout("rtl fill expand");
      search_results->set_clip_children(true);
      search_results->set_anchor(Anchor::TOP);
      search_results->set_parent_anchor(Anchor::TOP);
      search_results->set_pos(0, 8 + search_bar->get_height() + 8);

      scrollbar = &search_results->add_child<ScrollBar>();
      scrollbar->set_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_parent_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_thumb_thickness(12);
      scrollbar->set_track_thickness(12);
      scrollbar->set_max_width(12);
      scrollbar->set_orientation(SliderOrientation::VERTICAL);
      scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) {
        target_scroll_px = scroll_offset;
      });

      scrollable_content = &search_results->add_child<Widget>();

      albums_container = &scrollable_content->add_child<PanelAlbums>();
      albums_container->get_props().playlist_ids = {};
      albums_container->get_props().panel_search_visible = false;
      albums_container->get_props().is_scrollable = false;
      albums_container->get_props().cover_width = 48;
      albums_container->get_props().cover_min_horizontal_spacing = 12;
      albums_container->get_props().cover_min_vertical_spacing = 32;

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
      button_close = &buttons->add_child<Button>(U"Close");
      button_close->on_press([this]() { close(); });
      checkbox_search_all_collections = &buttons->add_child<Checkbox>(U"Search all collections");
    }

    void update_search_results() {
      auto new_search_text = sanitize_query(search_bar->label.get_text());
      if (search_text == new_search_text) {
        return;
      }

      search_text = new_search_text;
      playlist_ids.clear();
      found_tracks.clear();
      if (new_search_text.empty()) {
        albums_container->get_props().playlist_ids = {};
        albums_container->set_height(0);
        for (auto&& t : tracks_container->get_children()) {
          t->set_marked_for_deletion(true);
        }
        scrollbar->set_content_size(0);
        scrollbar->set_page_size(0);
        scrollbar->set_scroll_offset(0);
        scroll_px = 0.1;
        target_scroll_px = 0.0;
        return;
      }
      if (checkbox_search_all_collections->is_checked()) {
        playlist_ids = db::search_playlists(search_text, MAX_PLAYLISTS);
        found_tracks = db::search_tracks(search_text, MAX_TRACKS);
      } else {
        playlist_ids = db::search_playlists(search_text, collection_id, MAX_PLAYLISTS);
        found_tracks = db::search_tracks(search_text, collection_id, MAX_TRACKS);
      }

      albums_container->get_props().playlist_ids = playlist_ids;
      if (playlist_ids.size() > 0) {
        albums_container->set_height((((48 + 12) * playlist_ids.size()) / albums_container->get_width() + 1) * (48 + 32));
      } else {
        albums_container->set_height(0);
      }

      for (auto&& t : tracks_container->get_children()) {
        t->set_marked_for_deletion(true);
      }

      i32 i = 1;
      i32 track_height = theme::get_prop("tracklist_track_height").as_i32(22);
      for (db::track_info& t : found_tracks) {
        auto& w = tracks_container->add_child<WidgetTrack>(t.track_id, i, i % 2 == 0);
        w.set_min_height(track_height);
        w.set_max_height(track_height);
        i += 1;
      }

      i32 track_total_height = found_tracks.size() * track_height;
      i32 content_size = albums_container->get_height() + track_total_height;
      i32 page_size = scrollable_content->get_height();
      scrollbar->set_content_size(content_size);
      scrollbar->set_page_size(page_size);
      scrollbar->set_scroll_offset(0);
      scroll_px = 0.1;
      target_scroll_px = 0.0;
    }

    void update() override {
      set_width(std::clamp(ui.get_window_width() - 100, 300, 600));
      set_height(std::clamp(ui.get_window_height() - 100, 300, 500));

      if (scroll_px != target_scroll_px) {
        double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
        scroll_px = std::lerp(scroll_px, target_scroll_px, t);
        albums_container->set_y(-scroll_px);
        tracks_container->set_y(-scroll_px + albums_container->get_height());
        ui.mark_dirty_recursive(scrollable_content);
      }

      search_bar->set_width(get_width() - 16);

      search_results->set_width(get_width() - 16);
      search_results->set_height(get_height() - (22 + 32 + 4 * 8));

      buttons->set_width(get_width() - 16);

      albums_container->set_width(scrollable_content->get_width());
      tracks_container->set_width(scrollable_content->get_width());

      Popup::update();
    }

    void handle_event(Input::InputEventMouseScroll& e) override {
      if (is_mouse_hovering()) {
        scrollbar->scroll(e.offset.y);
        e.handled = true;
      }
    }

  protected:
    TextInput* search_bar{};
    Widget* search_results{};
    Widget* scrollable_content{};
    ScrollBar* scrollbar{};
    PanelAlbums* albums_container{};
    Widget* tracks_container{};
    Widget* buttons{};
    Button* button_close{};
    Checkbox* checkbox_search_all_collections{};

    size_t collection_id{};
    std::u32string search_text{};
    std::vector<size_t> playlist_ids;
    std::vector<db::track_info> found_tracks;
    double scroll_px{};
    double target_scroll_px{};

    static constexpr size_t MAX_PLAYLISTS = 15;
    static constexpr size_t MAX_TRACKS = 40;
};
