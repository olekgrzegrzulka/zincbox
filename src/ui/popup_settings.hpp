#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "common/types.hpp"
#include "core/player/equalizer.hpp"
#include "core/settings.hpp"
#include "ui/popup.hpp"

class PopupController;
class ScrollableView;
class ZincboxButton;

namespace zincgui {
  class Button;
  class Checkbox;
  class ComboBox;
  class Label;
  class Root;
  class Slider;
  class Spinner;
  class TextInput;
  class Widget;
} // namespace zincgui

class PopupSettings final : public Popup {
  public:
    PopupSettings(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_);

    void update() override;

  public:
    std::function<void(Settings)> on_save;
    std::function<void()> on_cancel;

  private:
    struct SettingEntry {
        zincgui::Widget* row_widget{};
        std::string search_blob;
    };

    struct SectionGroup {
        std::string raw_title_key;
        std::string localized_title;
        std::string search_blob;
        zincgui::Label* header_label{};
        zincgui::Widget* spacer_widget{};
        std::vector<SettingEntry> entries;
    };

    static constexpr size_t SECTION_COUNT = 4;

    Settings m_initial_settings{};
    Settings m_working_settings{};

    zincgui::TextInput* m_search_bar{};
    ScrollableView* m_scroll_view{};
    zincgui::Button* m_btn_save{};
    std::array<ZincboxButton*, SECTION_COUNT> m_page_buttons{};
    std::array<SectionGroup, SECTION_COUNT> m_sections{};
    std::array<zincgui::Slider*, player::EQ_BAND_COUNT> m_eq_sliders{};
    std::array<zincgui::Label*, player::EQ_BAND_COUNT> m_eq_value_labels{};
    std::array<zincgui::Label*, player::EQ_BAND_COUNT> m_eq_freq_labels{};
    std::string m_last_search_query;
    bool m_tight = false;

    static i32 control_row_height();
    static std::string make_search_blob(std::string_view raw_key, std::string_view localized_text);
    static std::string format_eq_freq(float freq);
    static std::string format_eq_db(float db);
    void build_header();
    void build_body();
    void build_footer();
    void begin_section(size_t section_idx);

    zincgui::ComboBox* add_row_combobox(size_t section_idx, std::string_view tr_key,
                                        std::function<void(const std::string&)> on_changed);

    zincgui::Spinner* add_row_spinner(size_t section_idx, std::string_view tr_key, i32 min_val, i32 max_val,
                                      i32 initial_val, std::string_view postfix, std::function<void(i32)> on_changed);

    zincgui::Checkbox* add_row_checkbox(size_t section_idx, std::string_view tr_key, bool initial_checked,
                                        std::function<void(bool)> on_changed);

    zincgui::Button* add_row_action_button(size_t section_idx, std::string_view label_tr_key,
                                           std::string_view btn_tr_key, std::function<void()> on_clicked);

    void build_general_section();
    void build_playback_section();
    void build_equalizer_widget(size_t section_idx);
    void build_interface_section();
    void build_system_section();
    void select_sidebar_button(size_t active_idx);
    void scroll_to_section(size_t section_idx);
    void filter_by_search();
    void refresh_save_button_state();
};
