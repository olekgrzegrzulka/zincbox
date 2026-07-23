#include <algorithm>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "common/utf.hpp"
#include "core/musicdb/musicdb.hpp"
#include "tr.hpp"
#include "ui/panel_albums.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/zb_widgets.hpp"
#include "ui_generic/button.hpp"

#include "ui_generic/label.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class PopupInput : public Popup {
  public:
    PopupInput(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {
      set_layout("ttb expand fit fill m:8 s:8");

      title = &add_child<Label>(U"");
      title->set_max_height(32);
      text_input = &add_child<TextInput>();
      text_input->set_max_height(24);

      buttons = &add_child<Widget>();
      buttons->set_layout("ltr fill fit expand m:0 s:8");
      buttons->set_max_height(32);

      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() {
        if (on_cancel_pressed) { on_cancel_pressed(); }
        close();
      });

      btn_ok = &buttons->add_child<Button>(tr::get("dialog.action.ok"));
      btn_ok->on_press([this]() {
        if (on_ok_pressed) { on_ok_pressed(); }
        close();
      });
    }

    Label* title{};
    TextInput* text_input{};
    Widget* buttons{};
    Button* btn_cancel{};
    Button* btn_ok{};

    std::function<void()> on_ok_pressed{};
    std::function<void()> on_cancel_pressed{};
};

class PopupConfirm : public Popup {
  public:
    PopupConfirm(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                 std::u32string_view content_)
      : Popup(ui_, controller_, std::move(on_close_)) {
      set_layout("ttb fill fit expand m:8 s:8");

      title = &add_child<Label>(U"");
      title->set_height(32);
      title->set_min_height(32);
      title->set_max_height(32);

      content = &add_child<Label>(content_);
      content->set_height(content->get_text_extents().y);
      content->set_min_height(content->get_text_extents().y);
      content->set_max_height(content->get_text_extents().y);

      buttons = &add_child<Widget>();
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);
      buttons->set_layout("ltr fill fit expand m:0 s:8");

      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() {
        if (on_cancel_pressed) { on_cancel_pressed(); }
        close();
      });

      btn_ok = &buttons->add_child<Button>(tr::get("dialog.action.ok"));
      btn_ok->on_press([this]() {
        if (on_ok_pressed) { on_ok_pressed(); }
        close();
      });
    }

    Label* title{};
    Label* content{};
    Widget* buttons{};
    Button* btn_cancel{};
    Button* btn_ok{};

    std::function<void()> on_ok_pressed{};
    std::function<void()> on_cancel_pressed{};
};

class PopupImportFolders : public Popup {
  public:
    PopupImportFolders(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::span<std::string> dropped_directories)
      : Popup(ui_, controller_, std::move(on_close_)) {

      set_layout("ttb expand fit fill m:8 s:8");

      dirs = std::vector<std::string>(dropped_directories.begin(), dropped_directories.end());

      title = &add_child<Label>(U"");
      title->set_anchor(Anchor::TOP);
      title->set_parent_anchor(Anchor::TOP);
      title->set_height(32);
      title->set_min_height(32);
      title->set_max_height(32);

      content = &add_child<Label>(U"");
      content->set_anchor(Anchor::CENTER);
      content->set_parent_anchor(Anchor::CENTER);

      buttons = &add_child<Widget>();
      buttons->set_anchor(Anchor::BOTTOM);
      buttons->set_parent_anchor(Anchor::BOTTOM);
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);

      buttons->set_layout("ltr fill fit expand m:0 s:8");

      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() { close(); });

      std::string content_str;
      i32 index = 1;
      for (const auto& dir : dirs) {
        content_str += std::to_string(index) + ". " + dir + "\n";
        index += 1;
      }
      // remove the last newline
      if (!content_str.empty()) { content_str.pop_back(); }

      content->set_text(utf8_to_utf32(content_str));
      content->update();
      content->set_size(content->get_text_extents());
      content->set_min_height(content->get_text_extents().y);
      content->set_max_height(content->get_text_extents().y);
      set_width(std::max(content->get_width() + 16, 400));

      if (dirs.size() > 1) {
        auto str_size = std::to_string(dirs.size());
        title->set_text(tr::format("popup.import.title_plural", str_size));

        btn_add_collections = &buttons->add_child<Button>(tr::format("popup.import.action_add_collections", str_size));
        btn_add_collections->on_press([this]() {
          if (on_add_collections_pressed) { on_add_collections_pressed(dirs); }
          close();
        });

        btn_merge = &buttons->add_child<Button>(tr::get("popup.import.action_merge"));
        btn_merge->on_press([this]() {
          if (on_merge_pressed) { on_merge_pressed(dirs); }
          close();
        });
      } else if (dirs.size() == 1) {
        title->set_text(tr::get("popup.import.title_single"));

        btn_add_collections = &buttons->add_child<Button>(tr::get("popup.import.action_add_collection"));
        btn_add_collections->on_press([this]() {
          if (on_add_collections_pressed) { on_add_collections_pressed(dirs); }
          close();
        });
      }
    }

    std::vector<std::string> dirs;
    Label* title{};
    Label* content{};
    Widget* buttons{};
    Button* btn_cancel{};
    Button* btn_add_collections{};
    Button* btn_merge{};

    std::function<void(const std::vector<std::string>&)> on_add_collections_pressed{};
    std::function<void(const std::vector<std::string>&)> on_merge_pressed{};
};

class PopupSetSources : public Popup {
  public:
    PopupSetSources(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_, size_t collection_id_)
      : Popup(ui_, controller_, std::move(on_close_)), collection_id(collection_id_) {

      set_layout("ttb expand fit m:8 s:8");

      auto& collection = db::collection_by_id(collection_id)->get();

      title = &add_child<Label>(tr::format("popup.sources.title", utf32_to_utf8(collection.name())));
      title->set_height(32);
      title->set_anchor(Anchor::TOP);
      title->set_parent_anchor(Anchor::TOP);

      content = &add_child<Widget>();
      content->set_clip_children(true);

      scrollable_content = &content->add_child<Widget>();
      scrollable_content->set_ignore_parents_layout(true);
      scrollable_content->set_clip_children(true);
      scrollable_content->set_layout("ttb s:0 m:0 expand fit");

      scrollbar = &content->add_child<ZincboxScrollbar>();
      scrollbar->set_ignore_parents_layout(true);
      scrollbar->set_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_parent_anchor(Anchor::TOP_RIGHT);
      scrollbar->set_thumb_thickness(10);
      scrollbar->set_track_thickness(10);
      scrollbar->set_width(10);
      scrollbar->set_orientation(SliderOrientation::VERTICAL);
      scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });

      buttons = &add_child<Widget>();
      buttons->set_anchor(Anchor::BOTTOM);
      buttons->set_parent_anchor(Anchor::BOTTOM);
      buttons->set_height(32);
      buttons->set_layout("ltr fill fit expand m:0 s:8");

      btn_close = &buttons->add_child<Button>(tr::get("dialog.action.close"));
      btn_close->on_press([this]() { close(); });

      btn_add_dir = &buttons->add_child<Button>(tr::get("dialog.action.add_directory"));
      btn_add_dir->on_press([this]() {
        if (on_add_dir_pressed) { on_add_dir_pressed(); }
        close();
      });

      if (!collection.paths().empty()) {
        for (i32 num = 1; const auto& path : collection.paths()) {
          std::u32string str = utf8_to_utf32(std::to_string(num)) + U". " + utf8_to_utf32(path);

          auto& container = scrollable_content->add_child<Sprite>(num % 2 == 0 ? "track_bg1" : "track_bg2");
          container.set_layout("ltr fill expand m:8 s:8");
          container.set_height(40);

          auto& label = container.add_child<Label>(str);
          label.set_label_anchor(Anchor::LEFT);
          label.set_min_width(0);
          label.set_max_width(0);

          auto& button = container.add_child<Button>(tr::get("dialog.action.remove"));
          button.set_min_width(60);
          button.set_max_width(60);
          button.on_press([this, path]() {
            if (on_remove_path_pressed) { on_remove_path_pressed(path); }
          });

          label.update();
          max_path_label_width = std::max<i32>(max_path_label_width, label.get_text_extents().x);

          num += 1;
        }
      } else {
        auto& label = add_child<Label>(tr::get("collection.sources.none"));
        label.set_text_color(theme::get_prop("text_color_muted").as_rgba());
        label.set_ignore_parents_layout(true);
        label.set_anchor(Anchor::CENTER);
        label.set_parent_anchor(Anchor::CENTER);
      }
    }

    void update() override {
      content->set_width(
        std::clamp<i32>(max_path_label_width + 60 + 24, 300, std::min(ui.get_window_width() - 100, 600)));

      if (max_path_label_width != 0) {
        content->set_height(
          std::clamp(std::min(scrollable_content->get_height(), ui.get_window_height() - 200), 100, 500));
      } else {
        content->set_height(60);
      }

      set_width(content->get_width() + 24);

      i32 scrollbar_width = scrollbar->get_is_drawn() ? scrollbar->get_width() : 0;
      scrollable_content->set_width(content->get_width() - scrollbar_width);
      scrollbar->set_height(content->get_height());
      scrollbar->set_content_size(scrollable_content->get_height());
      scrollbar->set_page_size(content->get_height());

      if (scroll_px != target_scroll_px) {
        double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
        scroll_px = std::lerp(scroll_px, target_scroll_px, t);
        scrollable_content->set_y(-std::floor(scroll_px));
        ui.mark_dirty_recursive(scrollable_content);
      }

      Popup::update();
    }

    size_t collection_id;
    Label* title{};
    Widget* content{};
    Widget* scrollable_content{};
    ScrollBar* scrollbar{};
    Widget* buttons{};
    Button* btn_close{};
    Button* btn_add_dir{};
    double scroll_px{};
    double target_scroll_px{};
    i32 max_path_label_width = 0;

    std::function<void()> on_add_dir_pressed{};
    std::function<void(const std::string&)> on_remove_path_pressed{};
};

class PopupAddToPlaylist : public Popup {
  public:
    PopupAddToPlaylist(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                       std::optional<size_t> track_id_)
      : Popup(ui_, controller_, std::move(on_close_)), track_id(track_id_) {

      std::u32string pretty_track;
      if (track_id.has_value() && db::track_by_id(track_id.value()).has_value()) {
        auto& track = db::track_by_id(track_id.value())->get();
        if (track.artist.empty() || track.title.empty()) {
          pretty_track = utf8_to_utf32(std::filesystem::path{track.path}.filename().string());
        } else {
          pretty_track = track.artist + U" - " + track.title;
        }
      }

      set_layout("ttb fill fit expand m:8 s:8");

      if (!pretty_track.empty()) {
        title = &add_child<Label>(tr::format("popup.add_to_playlist.title", utf32_to_utf8(pretty_track)));
      } else {
        title = &add_child<Label>(tr::format("popup.add_to_playlist.title_plural"));
      }
      title->set_height(32);
      title->set_min_height(32);
      title->set_max_height(32);
      title->set_anchor(Anchor::TOP);
      title->set_parent_anchor(Anchor::TOP);

      playlists_view = &add_child<PanelAlbums>();
      playlists_view->set_width((64 + 12) * 6);
      playlists_view->set_height(std::clamp(ui.get_window_height() - 300, 100, 500));
      playlists_view->set_min_height(playlists_view->get_height());
      playlists_view->set_max_height(playlists_view->get_height());
      playlists_view->props.collection_id = 0;
      playlists_view->props.button_sort_by_visible = false;
      playlists_view->props.button_add_playlist_visible = false;
      playlists_view->set_anchor(Anchor::CENTER);
      playlists_view->set_parent_anchor(Anchor::CENTER);

      playlists_view->on_playlist_lmb = [this](size_t playlist_id, Widget*) {
        if (on_playlist_selected) { on_playlist_selected(playlist_id); }
        close();
      };

      buttons = &add_child<Widget>();
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);
      buttons->set_anchor(Anchor::BOTTOM);
      buttons->set_parent_anchor(Anchor::BOTTOM);

      buttons->set_layout("ltr fill fit expand m:0 s:8");

      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() { close(); });

      set_width(playlists_view->get_width() + 16);
      set_height(playlists_view->get_height() + 100);
    }

    std::optional<size_t> track_id;
    Label* title{};
    Label* content{};
    PanelAlbums* playlists_view{};
    Widget* buttons{};
    Button* btn_cancel{};

    std::function<void(size_t)> on_playlist_selected{};
};

class PopupCreateSmartPlaylist : public Popup {
  public:
    PopupCreateSmartPlaylist(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {

      set_layout("ttb fit expand m:8 s:12");
      set_width(400);

      title = &add_child<Label>(tr::get("popup.smart_playlist.add.title"));

      Widget& container_name = add_child<Widget>();
      container_name.set_layout("ttb fit expand m:0 s:8");
      label_name = &container_name.add_child<Label>(tr::get("popup.smart_playlist.add.label_input"));
      input_name = &container_name.add_child<TextInput>();
      input_name->set_height(22);

      Widget& container_artists = add_child<Widget>();
      container_artists.set_layout("ttb fit expand m:0 s:8");
      label_artists = &container_artists.add_child<Label>(tr::get("popup.smart_playlist.add.label_artists"));
      input_artists = &container_artists.add_child<TextInput>();
      input_artists->set_height(22);

      buttons = &add_child<Widget>();
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);
      buttons->set_layout("ltr fill fit expand m:0 s:8");
      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() -> void {
        if (on_cancel) { on_cancel(); }
        close();
      });
      btn_add = &buttons->add_child<Button>(tr::get("dialog.action.add"));
      btn_add->on_press([this]() -> void {
        if (on_add) { on_add(); }
        close();
      });
    }

    Label* title{};
    Widget* buttons{};
    Button* btn_cancel{};
    Button* btn_add{};
    Label* label_artists{};
    TextInput* input_artists{};
    Label* label_name{};
    TextInput* input_name{};
    std::function<void()> on_cancel{};
    std::function<void()> on_add{};
    double scroll_px{};
    double target_scroll_px{};
};

class PopupAbout : public Popup {
  public:
    PopupAbout(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {
      set_layout("ttb fit expand m:8 s:12");
      set_width(450);

      auto& title = add_child<Label>(tr::get("popup.about.title"));
      title.set_height(32);
      title.set_resize_to_text_extents(false);
      auto& content =
        add_child<Label>(tr::format("popup.about.content", "0.1", __DATE__, "github.com/olekgrzegrzulka/zincbox"));
      content.set_text_color(theme::get_prop("text_color_muted").as_rgba());

      auto& buttons = add_child<Widget>();
      buttons.set_layout("ltr fill fit expand m:0 s:8");
      buttons.set_height(32);
      auto& btn_ok = buttons.add_child<Button>(tr::get("dialog.action.ok"));
      btn_ok.on_press([this]() -> void { close(); });
    }
};
