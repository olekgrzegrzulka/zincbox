#include <algorithm>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "common/logger.hpp"
#include "common/utf.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/settings.hpp"
#include "tr.hpp"
#include "ui/panel_albums.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/checkbox.hpp"
#include "ui_generic/combo_box.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/spinner.hpp"
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

class PopupSettings : public Popup {
  public:
    PopupSettings(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {

      set_size(600, 400);

      auto& buttons = add_child<Widget>();
      buttons.set_anchor(Anchor::BOTTOM);
      buttons.set_parent_anchor(Anchor::BOTTOM);
      buttons.set_width(width);
      auto& content = add_child<Widget>();
      content.set_anchor(Anchor::CENTER);
      content.set_parent_anchor(Anchor::CENTER);
      content.set_width(width);
      auto& title = add_child<Label>(tr::get("settings.title"));
      title.set_resize_to_text_extents(false);
      title.set_height(32);
      title.set_anchor(Anchor::TOP);
      title.set_parent_anchor(Anchor::TOP);
      title.set_width(width);
      content.set_height(height - (buttons.get_height() + title.get_height()));

      buttons.set_layout("ltr fill fit expand m:8 s:8");
      buttons.set_height(48);
      auto& btn_cancel = buttons.add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel.on_press([this]() -> void {
        if (on_cancel) { on_cancel(); }
        close();
      });

      auto& btn_save = buttons.add_child<Button>(tr::get("dialog.action.save"));
      btn_save.on_press([this]() -> void {
        if (on_save) { on_save(); }
        close();
      });

      content.set_layout("ltr expand fill m:8 s:12");

      auto& sidebar = content.add_child<Widget>();
      sidebar.set_layout("ttb expand m:0 s:0");
      sidebar.set_max_width(120);

      auto& page_general = content.add_child<Widget>();
      page_general.set_layout("ttb expand m:8 s:8");

      auto& page_playback = content.add_child<Widget>();
      page_playback.set_layout("ttb expand m:8 s:8");
      page_playback.set_is_drawn(false);
      page_playback.set_is_updated(false);

      auto& page_interface = content.add_child<Widget>();
      page_interface.set_layout("ttb expand m:8 s:8");
      page_interface.set_is_drawn(false);
      page_interface.set_is_updated(false);

      auto& btn_page_general = sidebar.add_child<Button>(tr::get("settings.category.general"));
      auto& btn_page_playback = sidebar.add_child<Button>(tr::get("settings.category.playback"));
      auto& btn_page_interface = sidebar.add_child<Button>(tr::get("settings.category.interface"));

      btn_page_general.on_press([&page_general, &page_interface, &page_playback]() {
        page_general.set_is_drawn(true);
        page_general.set_is_updated(true);
        page_interface.set_is_drawn(false);
        page_interface.set_is_updated(false);
        page_playback.set_is_drawn(false);
        page_playback.set_is_updated(false);
      });

      btn_page_playback.on_press([&page_general, &page_interface, &page_playback]() {
        page_general.set_is_drawn(false);
        page_general.set_is_updated(false);
        page_interface.set_is_drawn(false);
        page_interface.set_is_updated(false);
        page_playback.set_is_drawn(true);
        page_playback.set_is_updated(true);
      });

      btn_page_interface.on_press([&page_general, &page_interface, &page_playback]() {
        page_general.set_is_drawn(false);
        page_general.set_is_updated(false);
        page_interface.set_is_drawn(true);
        page_interface.set_is_updated(true);
        page_playback.set_is_drawn(false);
        page_playback.set_is_updated(false);
      });

      rgba text_color_muted = theme::get_prop("text_color_muted").as_rgba();

      // -----------------------------------------------
      //                     GENERAL
      // -----------------------------------------------
      auto& covers_source_label = page_general.add_child<Label>(tr::get("settings.playback.source_label"));
      covers_source_label.set_resize_to_text_extents(false);
      covers_source_label.set_height(16);
      covers_source_label.set_text_color(text_color_muted);

      covers_source_combo = &page_general.add_child<ComboBox>();
      covers_source_combo->add_item(tr::get("settings.playback.source_album"));
      covers_source_combo->add_item(tr::get("settings.playback.source_playlist"));
      ;

      auto& volume_bar_scroll_step_label = page_general.add_child<Label>(tr::get("settings.playback.volume_step"));
      volume_bar_scroll_step_label.set_resize_to_text_extents(false);
      volume_bar_scroll_step_label.set_height(16);
      volume_bar_scroll_step_label.set_text_color(text_color_muted);

      volume_bar_scroll_step_slider = &page_general.add_child<Slider>();
      volume_bar_scroll_step_slider->set_min_value(1);
      volume_bar_scroll_step_slider->set_max_value(10);
      volume_bar_scroll_step_slider->set_value(5);
      volume_bar_scroll_step_slider->on_value_changed([](float, float value) { out::info("{}", value); });

      // -----------------------------------------------
      //                    INTERFACE
      // -----------------------------------------------
      auto& theme_label = page_interface.add_child<Label>(tr::get("settings.interface.theme_label"));
      theme_label.set_resize_to_text_extents(false);
      theme_label.set_height(16);
      theme_label.set_text_color(text_color_muted);

      theme_combo = &page_interface.add_child<ComboBox>();
      for (auto& theme : theme::get_themes()) {
        theme_combo->add_item(utf8_to_utf32(theme));
      }

      theme_combo->on_item_selected(
        [this](i32) { out::info("theme_combo: {}", utf32_to_utf8(theme_combo->get_selected_item())); });

      auto& language_label = page_interface.add_child<Label>(tr::get("settings.interface.language_label"));
      language_label.set_resize_to_text_extents(false);
      language_label.set_height(16);
      language_label.set_text_color(text_color_muted);

      language_combo = &page_interface.add_child<ComboBox>();
      for (auto& language : theme::get_languages()) {
        language_combo->add_item(utf8_to_utf32(language));
      }

      language_combo->on_item_selected(
        [this](i32) { out::info("language_combo: {}", utf32_to_utf8(language_combo->get_selected_item())); });

      auto& interface_scale_label = page_interface.add_child<Label>(tr::get("settings.interface.interface_scale"));
      interface_scale_label.set_resize_to_text_extents(false);
      interface_scale_label.set_height(16);
      interface_scale_label.set_text_color(text_color_muted);
      interface_scale_spinner = &page_interface.add_child<Spinner>();
      interface_scale_spinner->set_postfix(U"%");
      interface_scale_spinner->set_min_value(50);
      interface_scale_spinner->set_max_value(200);
      interface_scale_spinner->set_value(100);

      auto& font_size_label = page_interface.add_child<Label>(tr::get("settings.interface.font_size"));
      font_size_label.set_resize_to_text_extents(false);
      font_size_label.set_height(16);
      font_size_label.set_text_color(text_color_muted);
      font_size_spinner = &page_interface.add_child<Spinner>();
      font_size_spinner->set_postfix(U"px");
      font_size_spinner->set_min_value(8);
      font_size_spinner->set_max_value(32);
      font_size_spinner->set_value(12);

      // -----------------------------------------------
      //                    PLAYBACK
      // -----------------------------------------------
      auto& shuffle_title = page_playback.add_child<Label>(tr::get("settings.playback.shuffle"));
      shuffle_title.set_resize_to_text_extents(false);
      shuffle_title.set_height(16);
      shuffle_title.set_text_color(text_color_muted);
      checkbox_shuffle_allow_same_album =
        &page_playback.add_child<Checkbox>(tr::get("settings.playback.allow_same_album"));
      checkbox_shuffle_allow_same_album->set_height(16);

      checkbox_shuffle_allow_same_artist =
        &page_playback.add_child<Checkbox>(tr::get("settings.playback.allow_same_artist"));
      checkbox_shuffle_allow_same_artist->set_height(16);
    }

    void load_settings(const settings& settings) {
      covers_source_combo->select_item_by_index(static_cast<i32>(settings.cover_preference));
      volume_bar_scroll_step_slider->set_value(settings.volume_step);
      theme_combo->select_item_by_label(utf8_to_utf32(settings.theme));
      language_combo->select_item_by_label(utf8_to_utf32(settings.language));
      interface_scale_spinner->set_value(settings.interface_scale);
      font_size_spinner->set_value(settings.font_size);
      checkbox_shuffle_allow_same_album->set_checked(settings.shuffle_allow_same_album);
      checkbox_shuffle_allow_same_artist->set_checked(settings.shuffle_allow_same_artist);
    }

    settings get_settings() const {
      settings s;
      s.cover_preference = static_cast<settings::CoverPreference>(covers_source_combo->get_selected_index());
      s.volume_step = volume_bar_scroll_step_slider->get_value();
      s.theme = utf32_to_utf8(theme_combo->get_selected_item());
      s.language = utf32_to_utf8(language_combo->get_selected_item());
      s.interface_scale = interface_scale_spinner->get_value();
      s.font_size = font_size_spinner->get_value();
      s.shuffle_allow_same_album = checkbox_shuffle_allow_same_album->is_checked();
      s.shuffle_allow_same_artist = checkbox_shuffle_allow_same_artist->is_checked();
      return s;
    }

  public:
    std::function<void()> on_save{};
    std::function<void()> on_cancel{};

  protected:
    ComboBox* covers_source_combo{};
    Slider* volume_bar_scroll_step_slider{};
    ComboBox* theme_combo{};
    ComboBox* language_combo{};
    Spinner* interface_scale_spinner{};
    Spinner* font_size_spinner{};
    Checkbox* checkbox_shuffle_allow_same_album{};
    Checkbox* checkbox_shuffle_allow_same_artist{};
};
