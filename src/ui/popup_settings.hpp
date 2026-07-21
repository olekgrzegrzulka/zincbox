#pragma once
#include <array>
#include <functional>
#include <unordered_map>
#include "common/utf.hpp"
#include "core/settings.hpp"
#include "tr.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
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
        if (on_save) { on_save(); }
        close();
      });

      auto& sidebar = content.add_child<Sprite>("panel_dark");
      sidebar.set_layout("ttb expand m:2 s:2");
      sidebar.set_max_width(120);

      std::array<std::u32string, 3> page_names = {tr::get("settings.category.general"),
                                                  tr::get("settings.category.playback"),
                                                  tr::get("settings.category.interface")};

      for (size_t i = 0; i < pages.size(); i += 1) {
        pages[i] = &content.add_child<Sprite>("panel_dark");
        pages[i]->set_layout("ttb m:8 s:8 left");
        pages[i]->set_is_drawn(false);
        pages[i]->set_is_updated(false);

        page_buttons[i] = &sidebar.add_child<Button>(page_names[i]);
        page_buttons[i]->set_switch_mode(true);
        page_buttons[i]->on_press([this, i]() -> void {
          for (size_t j = 0; j < pages.size(); j += 1) {
            pages[j]->set_is_drawn(i == j);
            pages[j]->set_is_updated(i == j);
            if (i != j) { page_buttons[j]->set_is_switched(false); }
          }
        });
      }

      page_buttons[0]->pressed();

      rgba text_color_muted = theme::get_prop("text_color_muted").as_rgba();

      auto create_widget_combobox = [this, &text_color_muted](Widget* parent,
                                                              std::pair<std::string, std::string> json_key,
                                                              std::u32string_view label_) -> ComboBox* {
        auto& label = parent->add_child<Label>(label_);
        label.set_resize_to_text_extents(false);
        label.set_height(16);
        label.set_text_color(text_color_muted);

        auto& combo = parent->add_child<ComboBox>();
        combo.set_width(128);

        auto& pad = parent->add_child<Widget>();
        pad.set_min_height(10);
        pad.set_max_height(10);

        combo_boxes[std::move(json_key)] = &combo;
        return &combo;
      };

      auto create_widget_spinner = [this, &text_color_muted](Widget* parent,
                                                             std::pair<std::string, std::string> json_key,
                                                             std::u32string_view label_) -> Spinner* {
        auto& label = parent->add_child<Label>(label_);
        label.set_resize_to_text_extents(false);
        label.set_height(16);
        label.set_text_color(text_color_muted);

        auto& spinner = parent->add_child<Spinner>();
        spinner.set_width(128);

        auto& pad = parent->add_child<Widget>();
        pad.set_min_height(10);
        pad.set_max_height(10);

        spinners[std::move(json_key)] = &spinner;
        return &spinner;
      };

      auto create_widget_checkbox = [this](Widget* parent, std::pair<std::string, std::string> json_key,
                                           std::u32string_view label_) -> Checkbox* {
        auto& checkbox = parent->add_child<Checkbox>(label_);
        checkbox.set_width(128);
        checkbox.set_height(24);

        // auto& pad = parent->add_child<Widget>();
        // pad.set_min_height(10);
        // pad.set_max_height(10);

        checkboxes[std::move(json_key)] = &checkbox;
        return &checkbox;
      };

      auto& page_general = *pages[0];
      auto& page_interface = *pages[1];
      auto& page_playback = *pages[2];

      // -----------------------------------------------
      //                     GENERAL
      // -----------------------------------------------

      auto* combo_cover_preference = create_widget_combobox(&page_general, {"general", "cover_preference"},
                                                            tr::get("settings.playback.source_label"));
      combo_cover_preference->add_item(tr::get("settings.playback.source_album"));
      combo_cover_preference->add_item(tr::get("settings.playback.source_playlist"));

      auto* spinner_volume_step =
        create_widget_spinner(&page_general, {"general", "volume_step"}, tr::get("settings.playback.volume_step"));
      spinner_volume_step->set_min_value(1);
      spinner_volume_step->set_max_value(10);
      spinner_volume_step->set_value(5);
      spinner_volume_step->on_value_changed(
        [spinner_volume_step]() { out::info("{}", spinner_volume_step->get_value()); });

      // -----------------------------------------------
      //                    INTERFACE
      // -----------------------------------------------
      auto* combo_theme =
        create_widget_combobox(&page_interface, {"interface", "theme"}, tr::get("settings.interface.theme_label"));
      for (auto& theme : theme::get_themes()) {
        combo_theme->add_item(utf8_to_utf32(theme));
      }
      auto* language_combo = create_widget_combobox(&page_interface, {"interface", "language"},
                                                    tr::get("settings.interface.language_label"));
      for (auto& language : theme::get_languages()) {
        language_combo->add_item(utf8_to_utf32(language));
      }

      auto* spinner_interface_scale = create_widget_spinner(&page_interface, {"interface", "ui_scale"},
                                                            tr::get("settings.interface.interface_scale"));
      spinner_interface_scale->set_postfix(U"%");
      spinner_interface_scale->set_min_value(50);
      spinner_interface_scale->set_max_value(200);
      spinner_interface_scale->set_value(100);

      auto* spinner_font_size =
        create_widget_spinner(&page_interface, {"interface", "font_size"}, tr::get("settings.interface.font_size"));
      spinner_font_size->set_postfix(U"px");
      spinner_font_size->set_min_value(8);
      spinner_font_size->set_max_value(32);
      spinner_font_size->set_value(12);

      // -----------------------------------------------
      //                    PLAYBACK
      // -----------------------------------------------
      auto& shuffle_title = page_playback.add_child<Label>(tr::get("settings.playback.shuffle"));
      shuffle_title.set_resize_to_text_extents(false);
      shuffle_title.set_height(16);
      shuffle_title.set_text_color(text_color_muted);
      auto* checkbox_shuffle_allow_same_album =
        create_widget_checkbox(&page_playback, std::pair{"playback", "shuffle_allow_same_album"},
                               tr::get("settings.playback.allow_same_album"));
      auto* checkbox_shuffle_allow_same_artist =
        create_widget_checkbox(&page_playback, std::pair{"playback", "shuffle_allow_same_artist"},
                               tr::get("settings.playback.allow_same_artist"));
      auto* checkbox_restart_on_previous = create_widget_checkbox(
        &page_playback, std::pair{"playback", "restart_on_previous"}, tr::get("settings.playback.restart_on_previous"));
    }

    void load_settings(const settings& settings) {
      const jt::Json& json = settings.to_json();

      for (auto& [key, combo] : combo_boxes) {
        if (json.contains(key.first) && json[key.first].contains(key.second) &&
            json[key.first][key.second].isString()) {
          //
          combo->select_item_by_label(utf8_to_utf32(json[key.first][key.second].getString()));
        }
      }

      for (auto& [key, spinner] : spinners) {
        if (json.contains(key.first) && json[key.first].contains(key.second) &&
            json[key.first][key.second].isNumber()) {
          //
          spinner->set_value(json[key.first][key.second].getNumber());
        }
      }

      for (auto& [key, checkbox] : checkboxes) {
        if (json.contains(key.first) && json[key.first].contains(key.second) && json[key.first][key.second].isBool()) {
          //
          checkbox->set_checked(json[key.first][key.second].getBool());
        }
      }
    }

    settings save_settings(const settings& base) const {
      jt::Json json = base.to_json();

      for (auto& [key, combo] : combo_boxes) {
        if (json[key.first][key.second].isString()) {
          json[key.first][key.second] = utf32_to_utf8(combo->get_selected_item());
        }
      }

      for (auto& [key, spinner] : spinners) {
        if (json[key.first][key.second].isNumber()) {
          //
          json[key.first][key.second] = spinner->get_value();
        }
      }

      for (auto& [key, checkbox] : checkboxes) {
        if (json[key.first][key.second].isBool()) {
          //
          json[key.first][key.second] = checkbox->is_checked();
        }
      }

      settings new_settings;
      new_settings.from_json(json);
      return new_settings;
    }

  public:
    std::function<void()> on_save{};
    std::function<void()> on_cancel{};

  protected:
    std::array<Widget*, 3> pages{};
    std::array<Button*, 3> page_buttons{};
    std::map<std::pair<std::string, std::string>, ComboBox*> combo_boxes;
    std::map<std::pair<std::string, std::string>, Spinner*> spinners;
    std::map<std::pair<std::string, std::string>, Checkbox*> checkboxes;
};
