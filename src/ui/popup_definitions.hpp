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
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
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

      set_layout("ttb expand fit fill m:8 s:8");

      auto& collection = db::collection_by_id(collection_id)->get();

      title = &add_child<Label>(tr::format("popup.sources.title", utf32_to_utf8(collection.name())));
      title->set_height(32);
      title->set_min_height(32);
      title->set_max_height(32);
      title->set_anchor(Anchor::TOP);
      title->set_parent_anchor(Anchor::TOP);

      list_container = &add_child<Widget>();
      list_container->set_layout("ltr s:0 m:0 fit");
      list_container->set_anchor(Anchor::CENTER);
      list_container->set_parent_anchor(Anchor::CENTER);

      left_panel = &list_container->add_child<Widget>();
      left_panel->set_layout("ttb s:4 m:0 fit");
      left_panel->set_clip_children(true);

      right_panel = &list_container->add_child<Widget>();
      right_panel->set_layout("ttb s:4 m:0 fit");

      buttons = &add_child<Widget>();
      buttons->set_anchor(Anchor::BOTTOM);
      buttons->set_parent_anchor(Anchor::BOTTOM);
      buttons->set_height(32);
      buttons->set_min_height(32);
      buttons->set_max_height(32);
      buttons->set_layout("ltr fill fit expand m:0 s:8");

      btn_cancel = &buttons->add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel->on_press([this]() { close(); });

      btn_add_dir = &buttons->add_child<Button>(tr::get("dialog.action.add_directory"));
      btn_add_dir->on_press([this]() {
        if (on_add_dir_pressed) { on_add_dir_pressed(); }
        close();
      });

      if (!collection.paths().empty()) {
        left_panel->set_width(310);
        right_panel->set_width(65);

        i32 num = 1;
        for (const auto& path : collection.paths()) {
          std::u32string str = utf8_to_utf32(std::to_string(num)) + U". " + utf8_to_utf32(path) + U"\n";

          auto& label = left_panel->add_child<Label>(str);
          label.set_resize_to_text_extents(false);
          label.set_size(310, 30);
          label.set_min_height(30);
          label.set_max_height(30);
          label.set_label_anchor(Anchor::CENTER_LEFT);

          auto& button = right_panel->add_child<Button>(tr::get("dialog.action.remove"));
          button.set_size(65, 30);
          button.set_min_height(30);
          button.set_max_height(30);
          button.on_press([this, path]() {
            if (on_remove_path_pressed) { on_remove_path_pressed(path); }
          });
          num += 1;
        }
      } else {
        left_panel->set_width(375);
        right_panel->set_width(0);
        auto& label = left_panel->add_child<Label>(tr::get("collection.sources.none"));
        label.set_text_color(theme::get_prop("text_color_muted").as_rgba());
      }

      left_panel->update();
      right_panel->update();
      list_container->update();
      list_container->set_height(std::max(64, left_panel->get_height()));
      list_container->set_min_height(list_container->get_height());
      list_container->set_max_height(list_container->get_height());
      set_width(std::max(list_container->get_width() + 16, 400));
    }

    size_t collection_id;
    Label* title{};
    Widget* list_container{};
    Widget* left_panel{};
    Widget* right_panel{};
    Widget* buttons{};
    Button* btn_cancel{};
    Button* btn_add_dir{};

    std::function<void()> on_add_dir_pressed{};
    std::function<void(const std::string&)> on_remove_path_pressed{};
};

class PopupAddToPlaylist : public Popup {
  public:
    PopupAddToPlaylist(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_, size_t track_id_)
      : Popup(ui_, controller_, std::move(on_close_)), track_id(track_id_) {

      auto& track = db::track_by_id(track_id)->get();
      std::u32string pretty_track;
      if (track.artist.empty() || track.title.empty()) {
        pretty_track = utf8_to_utf32(std::filesystem::path{track.path}.filename().string());
      } else {
        pretty_track = track.artist + U" - " + track.title;
      }

      set_layout("ttb fill fit expand m:8 s:8");

      title = &add_child<Label>(tr::format("popup.add_to_playlist.title", utf32_to_utf8(pretty_track)));
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

    size_t track_id;
    Label* title{};
    Label* content{};
    PanelAlbums* playlists_view{};
    Widget* buttons{};
    Button* btn_cancel{};

    std::function<void(size_t)> on_playlist_selected{};
};
