#pragma once
#include <array>
#include <functional>
#include <map>
#include "common/utf.hpp"
#include "core/settings.hpp"
#include "core/zincbox.hpp"
#include "tr.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/scrollable_view.hpp"
#include "ui/theme.hpp"
#include "ui_generic/checkbox.hpp"
#include "ui_generic/combo_box.hpp"
#include "ui_generic/spinner.hpp"
#include "ui_generic/ui.hpp"

class PopupSettings : public Popup {
  public:
    PopupSettings(UI& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
      : Popup(ui_, controller_, std::move(on_close_)) {

      static constexpr i32 TITLE_BAR_HEIGHT = 32;

      set_size(600, 400);

      auto& buttons = add_child<Widget>();
      buttons.set_anchor(Anchor::BOTTOM);
      buttons.set_parent_anchor(Anchor::BOTTOM);
      buttons.set_width(width);
      buttons.set_layout("ltr fill fit expand m:8 s:8");
      buttons.set_height(48);

      auto& content = add_child<Widget>();
      content.set_layout("ltr expand fill mx:8 my:0 s:8");
      content.set_anchor(Anchor::TOP);
      content.set_parent_anchor(Anchor::TOP);
      content.set_size(width, height - (48 + TITLE_BAR_HEIGHT));
      content.set_y(TITLE_BAR_HEIGHT);

      auto& title = add_child<Label>(tr::get("settings.title"));
      title.set_resize_to_text_extents(false);
      title.set_height(TITLE_BAR_HEIGHT);
      title.set_anchor(Anchor::TOP);
      title.set_parent_anchor(Anchor::TOP);
      title.set_width(width);

      auto& btn_cancel = buttons.add_child<Button>(tr::get("dialog.action.cancel"));
      btn_cancel.on_press([this]() -> void {
        if (on_cancel) { on_cancel(); }
        close();
      });

      auto& btn_save = buttons.add_child<Button>(tr::get("dialog.action.save"));
      btn_save.on_press([this]() -> void {
        if (on_save) { on_save(get_settings()); }
        close();
      });

      auto& sidebar = content.add_child<Sprite>("panel_dark");
      sidebar.set_layout("ttb expand m:2 s:2");
      sidebar.set_max_width(120);

      std::array<std::string, 3> page_names = {tr::get("settings.category.general"),
                                               tr::get("settings.category.playback"),
                                               tr::get("settings.category.interface")};

      for (size_t i = 0; i < pages.size(); i += 1) {
        pages[i] = &content.add_child<ScrollableView>();
        pages[i]->background()->set_texture("panel_dark");
        pages[i]->set_is_drawn(false);
        pages[i]->set_is_updated(false);

        page_buttons[i] = &sidebar.add_child<Button>(page_names[i]);
        page_buttons[i]->set_switch_mode(true);
        page_buttons[i]->on_press([this, i]() -> void {
          for (size_t j = 0; j < pages.size(); j += 1) {
            pages[j]->set_is_drawn(i == j);
            pages[j]->set_is_updated(i == j);
            pages[j]->update();
            if (i != j) { page_buttons[j]->set_is_switched(false); }
          }
        });
      }

      page_buttons[0]->pressed();

      rgba text_color_muted = theme::config().text_color_muted;

      auto create_widget_combobox = [this, &text_color_muted](Widget* parent_,
                                                              std::pair<std::string, std::string> json_key,
                                                              std::string_view label_) -> ComboBox* {
        auto& label = parent_->add_child<Label>(label_);
        label.set_resize_to_text_extents(false);
        label.set_height(16);
        label.set_text_color(text_color_muted);

        auto& combo = parent_->add_child<ComboBox>();
        combo.set_width(128);

        auto& pad = parent_->add_child<Widget>();
        pad.set_min_height(10);
        pad.set_max_height(10);

        combo_boxes[std::move(json_key)] = &combo;
        return &combo;
      };

      auto create_widget_spinner = [this, &text_color_muted](Widget* parent_,
                                                             std::pair<std::string, std::string> json_key,
                                                             std::string_view label_) -> Spinner* {
        auto& label = parent_->add_child<Label>(label_);
        label.set_resize_to_text_extents(false);
        label.set_height(16);
        label.set_text_color(text_color_muted);

        auto& spinner = parent_->add_child<Spinner>();
        spinner.set_width(128);

        auto& pad = parent_->add_child<Widget>();
        pad.set_min_height(10);
        pad.set_max_height(10);

        spinners[std::move(json_key)] = &spinner;
        return &spinner;
      };

      auto create_widget_checkbox = [this](Widget* parent_, std::pair<std::string, std::string> json_key,
                                           std::string_view label_) -> Checkbox* {
        auto& checkbox = parent_->add_child<Checkbox>(label_);
        checkbox.set_width(128);
        checkbox.set_height(24);

        checkboxes[std::move(json_key)] = &checkbox;
        return &checkbox;
      };

      auto& page_general = *pages[0];
      auto& page_playback = *pages[1];
      auto& page_interface = *pages[2];

      // -----------------------------------------------
      //                     GENERAL
      // -----------------------------------------------

      auto* combo_cover_preference = create_widget_combobox(page_general.content(), {"general", "cover_preference"},
                                                            tr::get("settings.playback.source_label"));
      combo_cover_preference->add_item("album", tr::get("settings.playback.source_album"));
      combo_cover_preference->add_item("playlist", tr::get("settings.playback.source_playlist"));

      auto* spinner_volume_step = create_widget_spinner(page_general.content(), {"general", "volume_step"},
                                                        tr::get("settings.playback.volume_step"));
      spinner_volume_step->set_postfix("%");
      spinner_volume_step->set_min_value(1);
      spinner_volume_step->set_max_value(10);
      spinner_volume_step->set_value(5);

      // -----------------------------------------------
      //                    PLAYBACK
      // -----------------------------------------------
      auto& shuffle_title = page_playback.content()->add_child<Label>(tr::get("settings.playback.shuffle"));
      shuffle_title.set_resize_to_text_extents(false);
      shuffle_title.set_height(16);
      shuffle_title.set_text_color(text_color_muted);
      create_widget_checkbox(page_playback.content(), std::pair{"playback", "shuffle_allow_same_album"},
                             tr::get("settings.playback.allow_same_album"));
      create_widget_checkbox(page_playback.content(), std::pair{"playback", "shuffle_allow_same_artist"},
                             tr::get("settings.playback.allow_same_artist"));

      auto& pad = page_playback.content()->add_child<Widget>();
      pad.set_min_height(10);
      pad.set_max_height(10);

      create_widget_checkbox(page_playback.content(), std::pair{"playback", "restart_on_previous"},
                             tr::get("settings.playback.restart_on_previous"));

      // -----------------------------------------------
      //                    INTERFACE
      // -----------------------------------------------
      auto* combo_theme = create_widget_combobox(page_interface.content(), {"interface", "theme"},
                                                 tr::get("settings.interface.theme_label"));
      combo_theme->add_item("default", tr::get("settings.interface.default_theme"));
      for (auto& theme : theme::get_themes()) {
        combo_theme->add_item(theme, theme);
      }
      auto* language_combo = create_widget_combobox(page_interface.content(), {"interface", "language"},
                                                    tr::get("settings.interface.language_label"));
      for (auto& language : theme::get_languages()) {
        language_combo->add_item(language, language);
      }

      auto* spinner_interface_scale =
        create_widget_spinner(page_interface.content(), {"interface", "scale"}, tr::get("settings.interface.scale"));
      spinner_interface_scale->set_postfix("%");
      spinner_interface_scale->set_min_value(75);
      spinner_interface_scale->set_max_value(200);
      spinner_interface_scale->set_value(100);

      auto* spinner_font_size = create_widget_spinner(page_interface.content(), {"interface", "font_size"},
                                                      tr::get("settings.interface.font_size"));
      spinner_font_size->set_postfix("px");
      spinner_font_size->set_min_value(8);
      spinner_font_size->set_max_value(32);
      spinner_font_size->set_value(12);

      auto* spinner_scrolling_speed = create_widget_spinner(page_interface.content(), {"interface", "scrolling_speed"},
                                                            tr::get("settings.interface.scrolling_speed"));
      spinner_scrolling_speed->set_min_value(10);
      spinner_scrolling_speed->set_max_value(150);
      spinner_scrolling_speed->set_value(12);

      load_settings();
    }

    void load_settings() {
      auto& s = zincbox::settings();

      if (s.general.cover_preference == Settings::CoverPreference::Album) {
        combo_boxes.at({"general", "cover_preference"})->select_item_by_id("album");
      } else {
        combo_boxes.at({"general", "cover_preference"})->select_item_by_id("playlist");
      }
      spinners.at({"general", "volume_step"})->set_value(s.general.volume_step);

      checkboxes.at({"playback", "shuffle_allow_same_album"})->set_checked(s.playback.shuffle_allow_same_album);
      checkboxes.at({"playback", "shuffle_allow_same_artist"})->set_checked(s.playback.shuffle_allow_same_artist);
      checkboxes.at({"playback", "restart_on_previous"})->set_checked(s.playback.restart_on_previous);

      combo_boxes.at({"interface", "theme"})->select_item_by_id(s.interface.theme);
      combo_boxes.at({"interface", "language"})->select_item_by_id(s.interface.language);
      spinners.at({"interface", "scale"})->set_value(s.interface.scale);
      spinners.at({"interface", "font_size"})->set_value(s.interface.font_size);
      spinners.at({"interface", "scrolling_speed"})->set_value(static_cast<i32>(s.interface.scrolling_speed));
    }

    [[nodiscard]] Settings get_settings() const {
      Settings s{};

      if (combo_boxes.at({"general", "cover_preference"})->get_selected_item_id() == "album") {
        s.general.cover_preference = Settings::CoverPreference::Album;
      } else {
        s.general.cover_preference = Settings::CoverPreference::Playlist;
      }
      s.general.volume_step = spinners.at({"general", "volume_step"})->get_value();

      s.playback.shuffle_allow_same_album = checkboxes.at({"playback", "shuffle_allow_same_album"})->is_checked();
      s.playback.shuffle_allow_same_artist = checkboxes.at({"playback", "shuffle_allow_same_artist"})->is_checked();
      s.playback.restart_on_previous = checkboxes.at({"playback", "restart_on_previous"})->is_checked();

      s.interface.theme = combo_boxes.at({"interface", "theme"})->get_selected_item_id();
      s.interface.language = combo_boxes.at({"interface", "language"})->get_selected_item_id();
      s.interface.scale = spinners.at({"interface", "scale"})->get_value();
      s.interface.font_size = spinners.at({"interface", "font_size"})->get_value();
      s.interface.scrolling_speed = static_cast<float>(spinners.at({"interface", "scrolling_speed"})->get_value());

      s.clamp_values();
      return s;
    }

  public:
    std::function<void(Settings)> on_save{};
    std::function<void()> on_cancel{};

  protected:
    std::array<ScrollableView*, 3> pages{};
    std::array<Button*, 3> page_buttons{};
    std::map<std::pair<std::string, std::string>, ComboBox*> combo_boxes;
    std::map<std::pair<std::string, std::string>, Spinner*> spinners;
    std::map<std::pair<std::string, std::string>, Checkbox*> checkboxes;
};
