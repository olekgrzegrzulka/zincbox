#include "popup_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "common/search_utils.hpp"
#include "core/io.hpp"
#include "core/player/equalizer.hpp"
#include "core/settings.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/scrollable_view.hpp"
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/checkbox.hpp"
#include "ui/zincgui/combo_box.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/spinner.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

PopupSettings::PopupSettings(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)), m_initial_settings(zincbox::settings()),
    m_working_settings(zincbox::settings()) {

  set_layout("ttb expand fill m:8 s:8");

  build_header();
  build_body();
  build_footer();

  build_general_section();
  build_playback_section();
  build_interface_section();
  build_system_section();

  select_sidebar_button(0);
  refresh_save_button_state();
}

void PopupSettings::update() {
  set_width(std::min<i32>(get_max_content_width(), static_cast<i32>(800 * zincbox::ui_scale())));
  set_height(std::min<i32>(get_max_content_height(), static_cast<i32>(600 * zincbox::ui_scale())));
  refresh_save_button_state();
  Popup::update();

  bool tight_ = width < 850;
  if (tight_ != m_tight) {
    m_tight = tight_;
    for (auto* label : m_eq_freq_labels) {
      label->set_is_drawn(!m_tight);
    }
    for (auto* label : m_eq_value_labels) {
      label->set_is_drawn(!m_tight);
    }
  }
}

i32 PopupSettings::control_row_height() { return static_cast<i32>((8 * zincbox::ui_scale()) + zincbox::font_size()); }

std::string PopupSettings::make_search_blob(std::string_view raw_key, std::string_view localized_text) {
  std::string combined = std::string(raw_key) + " " + std::string(localized_text);
  return sanitize_query(combined);
}

std::string PopupSettings::format_eq_freq(float freq) {
  if (freq >= 1000.0f) {
    float khz = freq / 1000.0f;
    float rounded_1dp = std::round(khz * 10.0f) / 10.0f;
    if (std::abs(rounded_1dp - std::round(rounded_1dp)) < 0.05f) {
      return std::format("{} kHz", static_cast<i32>(std::round(rounded_1dp)));
    }
    return std::format("{:.1f} kHz", rounded_1dp);
  }
  return std::format("{} Hz", static_cast<i32>(std::round(freq)));
}

std::string PopupSettings::format_eq_db(float db) {
  if (std::abs(db) < 0.05f) { return "0.0 dB"; }
  return std::format("{:.1f} dB", db);
}

void PopupSettings::build_header() {
  auto& title = add_child<zincgui::Label>(tr::get("settings.title"));
  title.set_resize_to_text_extents(false);
  title.set_fixed_height(static_cast<i32>(28 * zincbox::ui_scale()));
  title.set_anchor(zincgui::Anchor::TOP);
  title.set_parent_anchor(zincgui::Anchor::TOP);

  m_search_bar = &add_child<zincgui::TextInput>();
  m_search_bar->set_fixed_height(control_row_height());
  m_search_bar->set_on_text_changed([this]() -> void { filter_by_search(); });
}

void PopupSettings::build_body() {
  auto& content = add_child<zincgui::Widget>();
  content.set_layout("ltr expand fill m:0 s:16");
  content.set_weight(1.0f);

  auto& sidebar = content.add_child<zincgui::Sprite>("panel_dark");
  sidebar.set_layout("ttb expand m:0 s:0");
  sidebar.set_fixed_width(static_cast<i32>(130 * zincbox::ui_scale()));

  m_scroll_view = &content.add_child<ScrollableView>();
  m_scroll_view->set_weight(1.0f);
  m_scroll_view->background()->set_texture("panel_dark");
  m_scroll_view->content()->set_layout("ttb fit expand mx:12 my:8 s:6");

  constexpr std::array<std::string_view, SECTION_COUNT> CATEGORY_KEYS = {
    "settings.category.general", "settings.category.playback", "settings.category.interface",
    "settings.category.system"};

  for (size_t i = 0; i < SECTION_COUNT; i += 1) {
    std::string tex = (i % 2 == 0) ? "settings_page_button_odd" : "settings_page_button_even";

    m_sections[i].raw_title_key = std::string(CATEGORY_KEYS[i]);
    std::string localized = tr::get(m_sections[i].raw_title_key);
    m_sections[i].localized_title = localized;
    m_sections[i].search_blob = make_search_blob(CATEGORY_KEYS[i], localized);

    auto& btn = sidebar.add_child<ZincboxButton>(tex);
    btn.get_label().set_text(localized);
    btn.set_fixed_height(control_row_height());
    btn.set_switch_mode(false);
    btn.on_press([this, i]() -> void {
      select_sidebar_button(i);
      scroll_to_section(i);
    });
    m_page_buttons[i] = &btn;
  }
}

void PopupSettings::build_footer() {
  auto& buttons = add_child<zincgui::Widget>();
  buttons.set_anchor(zincgui::Anchor::BOTTOM);
  buttons.set_parent_anchor(zincgui::Anchor::BOTTOM);
  buttons.set_layout("ltr expand fill m:0 s:8");
  buttons.set_fixed_height(static_cast<i32>(32 * zincbox::ui_scale()));

  auto& btn_cancel = buttons.add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel.set_weight(1.0f);
  btn_cancel.on_press([this]() -> void {
    if (on_cancel) { on_cancel(); }
    close();
  });

  m_btn_save = &buttons.add_child<zincgui::Button>(tr::get("dialog.action.save"));
  m_btn_save->set_weight(1.0f);
  m_btn_save->on_press([this]() -> void {
    if (m_working_settings == m_initial_settings) { return; }
    m_working_settings.clamp_values();
    if (on_save) { on_save(m_working_settings); }
    close();
  });
}

void PopupSettings::begin_section(size_t section_idx) {
  auto* parent = m_scroll_view->content();
  if (section_idx > 0) {
    auto& spacer = parent->add_child<zincgui::Widget>();
    spacer.set_fixed_height(static_cast<i32>(12 * zincbox::ui_scale()));
    m_sections[section_idx].spacer_widget = &spacer;
  }

  auto& header = parent->add_child<zincgui::Label>(m_sections[section_idx].localized_title);
  header.set_resize_to_text_extents(false);
  header.set_fixed_height(static_cast<i32>(24 * zincbox::ui_scale()));
  header.set_text_color(theme::config().text_color_muted);
  header.set_label_anchor(zincgui::Anchor::LEFT);
  header.set_font_size(zincbox::font_size_lg());
  m_sections[section_idx].header_label = &header;
}

zincgui::ComboBox* PopupSettings::add_row_combobox(size_t section_idx, std::string_view tr_key,
                                                   std::function<void(const std::string&)> on_changed) {
  std::string localized = tr::get(std::string(tr_key));
  auto& row = m_scroll_view->content()->add_child<zincgui::Widget>();
  row.set_layout("ltr fill expand m:0 s:8");
  row.set_fixed_height(control_row_height());

  auto& label = row.add_child<zincgui::Label>(localized);
  label.set_resize_to_text_extents(false);
  label.set_label_anchor(zincgui::Anchor::LEFT);
  label.set_min_width(1);

  auto& combo = row.add_child<zincgui::ComboBox>();
  combo.set_fixed_height(control_row_height());
  combo.on_item_selected([combo_ = &combo, this, cb = std::move(on_changed)]() -> void {
    cb(combo_->get_selected_item_id());
    refresh_save_button_state();
  });

  m_sections[section_idx].entries.push_back(SettingEntry{
    .row_widget = &row,
    .search_blob = make_search_blob(tr_key, localized),
  });
  return &combo;
}

zincgui::Spinner* PopupSettings::add_row_spinner(size_t section_idx, std::string_view tr_key, i32 min_val, i32 max_val,
                                                 i32 initial_val, std::string_view postfix,
                                                 std::function<void(i32)> on_changed) {
  std::string localized = tr::get(std::string(tr_key));
  auto& row = m_scroll_view->content()->add_child<zincgui::Widget>();
  row.set_layout("ltr fill expand m:0 s:8");
  row.set_fixed_height(control_row_height());

  auto& label = row.add_child<zincgui::Label>(localized);
  label.set_resize_to_text_extents(false);
  label.set_label_anchor(zincgui::Anchor::LEFT);
  label.set_min_width(1);

  auto& spinner = row.add_child<zincgui::Spinner>();
  spinner.set_fixed_height(control_row_height());
  if (!postfix.empty()) { spinner.set_postfix(std::string(postfix)); }
  spinner.set_min_value(min_val);
  spinner.set_max_value(max_val);
  spinner.set_value(initial_val);
  spinner.on_value_changed([spinner_ = &spinner, this, cb = std::move(on_changed)]() -> void {
    cb(spinner_->get_value());
    refresh_save_button_state();
  });

  m_sections[section_idx].entries.push_back(SettingEntry{
    .row_widget = &row,
    .search_blob = make_search_blob(tr_key, localized),
  });
  return &spinner;
}

zincgui::Checkbox* PopupSettings::add_row_checkbox(size_t section_idx, std::string_view tr_key, bool initial_checked,
                                                   std::function<void(bool)> on_changed) {
  std::string localized = tr::get(std::string(tr_key));
  auto& checkbox = m_scroll_view->content()->add_child<zincgui::Checkbox>(localized);
  checkbox.set_fixed_height(control_row_height());
  checkbox.set_checked(initial_checked);
  checkbox.on_value_changed([checkbox_ = &checkbox, this, cb = std::move(on_changed)]() -> void {
    cb(checkbox_->is_checked());
    refresh_save_button_state();
  });
  checkbox.set_checkbox_anchor(zincgui::Anchor::LEFT);

  m_sections[section_idx].entries.push_back(SettingEntry{
    .row_widget = &checkbox,
    .search_blob = make_search_blob(tr_key, localized),
  });
  return &checkbox;
}

zincgui::Button* PopupSettings::add_row_action_button(size_t section_idx, std::string_view label_tr_key,
                                                      std::string_view btn_tr_key, std::function<void()> on_clicked) {
  std::string localized_label = tr::get(std::string(label_tr_key));
  std::string localized_btn = tr::get(std::string(btn_tr_key));

  auto& row = m_scroll_view->content()->add_child<zincgui::Widget>();
  row.set_layout("ltr fill expand m:0 s:8");
  row.set_fixed_height(control_row_height());

  auto& label = row.add_child<zincgui::Label>(localized_label);
  label.set_resize_to_text_extents(false);
  label.set_label_anchor(zincgui::Anchor::LEFT);
  label.set_min_width(1);

  auto& btn = row.add_child<zincgui::Button>(localized_btn);
  btn.set_fixed_height(control_row_height());
  btn.on_press(std::move(on_clicked));

  std::string combined_raw = std::string(label_tr_key) + " " + std::string(btn_tr_key);
  std::string combined_loc = localized_label + " " + localized_btn;
  m_sections[section_idx].entries.push_back(SettingEntry{
    .row_widget = &row,
    .search_blob = make_search_blob(combined_raw, combined_loc),
  });
  return &btn;
}

void PopupSettings::build_general_section() {
  constexpr size_t SEC = 0;
  begin_section(SEC);

  auto* combo_cover = add_row_combobox(SEC, "settings.playback.source_label", [this](const std::string& id) {
    m_working_settings.general.cover_preference =
      (id == "album") ? Settings::CoverPreference::ALBUM : Settings::CoverPreference::PLAYLIST;
  });
  combo_cover->add_item("album", tr::get("settings.playback.source_album"));
  combo_cover->add_item("playlist", tr::get("settings.playback.source_playlist"));
  if (m_working_settings.general.cover_preference == Settings::CoverPreference::ALBUM) {
    combo_cover->select_item_by_id("album");
  } else {
    combo_cover->select_item_by_id("playlist");
  }

  add_row_spinner(SEC, "settings.playback.volume_step", 1, 10, m_working_settings.general.volume_step, "%",
                  [this](i32 val) { m_working_settings.general.volume_step = val; });
}

void PopupSettings::build_playback_section() {
  constexpr size_t SEC = 1;
  begin_section(SEC);

  add_row_checkbox(SEC, "settings.playback.allow_same_album", m_working_settings.playback.shuffle_allow_same_album,
                   [this](bool checked) -> void { m_working_settings.playback.shuffle_allow_same_album = checked; });

  add_row_checkbox(SEC, "settings.playback.allow_same_artist", m_working_settings.playback.shuffle_allow_same_artist,
                   [this](bool checked) -> void { m_working_settings.playback.shuffle_allow_same_artist = checked; });

  add_row_checkbox(SEC, "settings.playback.restart_on_previous", m_working_settings.playback.restart_on_previous,
                   [this](bool checked) -> void { m_working_settings.playback.restart_on_previous = checked; });

  auto* crossfade = add_row_spinner(SEC, "settings.playback.crossfade", 0, 5000, m_working_settings.playback.crossfade,
                                    "ms", [this](i32 val) -> void { m_working_settings.playback.crossfade = val; });
  crossfade->set_button_step(25);

  build_equalizer_widget(SEC);
}

void PopupSettings::build_equalizer_widget(size_t section_idx) {
  std::string eq_title_loc = tr::get("settings.playback.equalizer");
  std::string eq_reset_loc = tr::get("settings.playback.equalizer_reset");

  auto& eq_wrapper = m_scroll_view->content()->add_child<zincgui::Widget>();
  eq_wrapper.set_layout("ttb fit expand m:0 s:6");

  auto& header_row = eq_wrapper.add_child<zincgui::Widget>();
  header_row.set_layout("ltr fill expand m:0 s:8");
  header_row.set_fixed_height(control_row_height());

  auto& eq_label = header_row.add_child<zincgui::Label>(eq_title_loc);
  eq_label.set_resize_to_text_extents(false);
  eq_label.set_label_anchor(zincgui::Anchor::LEFT);
  eq_label.set_min_width(1);

  auto& reset_btn = header_row.add_child<zincgui::Button>(eq_reset_loc);
  reset_btn.set_fixed_height(control_row_height());
  reset_btn.on_press([this]() -> void {
    for (size_t i = 0; i < player::EQ_BAND_COUNT; i += 1) {
      m_working_settings.playback.equalizer[i] = 0.0f;
      m_eq_sliders[i]->set_value(0.0f, false);
      m_eq_value_labels[i]->set_text(format_eq_db(0.0f));
    }
    refresh_save_button_state();
  });

  auto& bands_row = eq_wrapper.add_child<zincgui::Widget>();
  bands_row.set_layout("ltr fill expand m:4 s:4");
  bands_row.set_fixed_height(static_cast<i32>(150 * zincbox::ui_scale()));

  for (size_t i = 0; i < player::EQ_BAND_COUNT; i += 1) {
    auto& col = bands_row.add_child<zincgui::Widget>();
    col.set_layout("ttb fill expand m:0 s:4");

    float init_gain = std::clamp(m_working_settings.playback.equalizer[i], -12.0f, 12.0f);

    auto& val_label = col.add_child<zincgui::Label>(format_eq_db(init_gain));
    val_label.set_resize_to_text_extents(false);
    val_label.set_fixed_height(static_cast<i32>(16 * zincbox::ui_scale()));
    val_label.set_label_anchor(zincgui::Anchor::CENTER);
    m_eq_value_labels[i] = &val_label;

    auto& slider = col.add_child<ZincboxSlider>("equalizer");
    slider.set_orientation(zincgui::SliderOrientation::VERTICAL_INVERTED);
    slider.set_fixed_width(slider.get_width());
    slider.set_track_nine_slice_margin(3);
    slider.set_thumb_nine_slice_margin(6);
    slider.set_min_value(-12.0f);
    slider.set_max_value(12.0f);
    slider.set_sensitivity(1.0f);
    slider.set_value(init_gain, false);
    slider.on_value_changed([this, i](float, float new_val) -> void {
      m_working_settings.playback.equalizer[i] = std::clamp(new_val, -12.0f, 12.0f);
      m_eq_value_labels[i]->set_text(format_eq_db(m_working_settings.playback.equalizer[i]));
      refresh_save_button_state();
    });
    slider.set_is_scrollable(false);
    m_eq_sliders[i] = &slider;

    auto& freq_label = col.add_child<zincgui::Label>(format_eq_freq(player::EQ_FREQUENCIES[i]));
    freq_label.set_resize_to_text_extents(false);
    freq_label.set_fixed_height(static_cast<i32>(16 * zincbox::ui_scale()));
    freq_label.set_label_anchor(zincgui::Anchor::CENTER);
    freq_label.set_text_color(theme::config().text_color_muted);
    m_eq_freq_labels[i] = &freq_label;
  }

  m_sections[section_idx].entries.push_back(SettingEntry{
    .row_widget = &eq_wrapper,
    .search_blob = make_search_blob("settings.playback.equalizer settings.playback.equalizer_reset eq hz db",
                                    eq_title_loc + " " + eq_reset_loc),
  });
}

void PopupSettings::build_interface_section() {
  constexpr size_t SEC = 2;
  begin_section(SEC);

  auto* combo_theme = add_row_combobox(SEC, "settings.interface.theme_label",
                                       [this](const std::string& id) { m_working_settings.interface.theme = id; });
  combo_theme->add_item("default", tr::get("settings.interface.default_theme"));
  for (const auto& t : theme::get_themes()) {
    combo_theme->add_item(t, t);
  }
  combo_theme->select_item_by_id(m_working_settings.interface.theme);

  auto* combo_lang = add_row_combobox(SEC, "settings.interface.language_label",
                                      [this](const std::string& id) { m_working_settings.interface.language = id; });
  for (const auto& lang : theme::get_languages()) {
    combo_lang->add_item(lang, lang);
  }
  combo_lang->select_item_by_id(m_working_settings.interface.language);

  add_row_spinner(SEC, "settings.interface.scale", 75, 200, m_working_settings.interface.scale, "%",
                  [this](i32 val) -> void { m_working_settings.interface.scale = val; });

  add_row_spinner(SEC, "settings.interface.font_size", 8, 32, m_working_settings.interface.font_size, "px",
                  [this](i32 val) -> void { m_working_settings.interface.font_size = val; });

  add_row_spinner(SEC, "settings.interface.scrolling_speed", 10, 150,
                  static_cast<i32>(m_working_settings.interface.scrolling_speed), "",
                  [this](i32 val) -> void { m_working_settings.interface.scrolling_speed = static_cast<float>(val); });

  add_row_spinner(SEC, "settings.interface.cover_size", 16, 256, m_working_settings.interface.cover_size, "px",
                  [this](i32 val) -> void { m_working_settings.interface.cover_size = val; });
}

void PopupSettings::build_system_section() {
  constexpr size_t SEC = 3;
  begin_section(SEC);

  add_row_action_button(SEC, "settings.system.data_folder_label", "settings.system.open_data_folder",
                        []() -> void { io::open_folder_in_file_manager(io::get_user_data_path()); });

  add_row_action_button(SEC, "settings.system.config_file_label", "settings.system.open_config_file",
                        []() -> void { io::open_file_in_default_editor(io::get_cfg_path()); });
}

void PopupSettings::select_sidebar_button(size_t active_idx) {
  for (size_t j = 0; j < SECTION_COUNT; j += 1) {
    m_page_buttons[j]->set_is_switched(j == active_idx);
  }
}

void PopupSettings::scroll_to_section(size_t section_idx) {
  if (section_idx >= SECTION_COUNT) { return; }
  auto* header = m_sections[section_idx].header_label;
  if (header != nullptr && header->get_is_drawn()) { m_scroll_view->set_scroll_offset(std::max(0, header->get_y())); }
}

void PopupSettings::filter_by_search() {
  std::string query = sanitize_query(m_search_bar->label.get_text());
  if (query == m_last_search_query) { return; }
  m_last_search_query = query;

  for (size_t s = 0; s < SECTION_COUNT; s += 1) {
    auto& sec = m_sections[s];
    bool section_title_matches = !query.empty() && sec.search_blob.find(query) != std::string::npos;
    bool any_entry_visible = false;

    for (auto& entry : sec.entries) {
      bool visible = query.empty() || section_title_matches || entry.search_blob.find(query) != std::string::npos;
      entry.row_widget->set_is_drawn(visible);
      entry.row_widget->set_is_updated(visible);
      if (visible) { any_entry_visible = true; }
    }

    if (sec.header_label != nullptr) {
      sec.header_label->set_is_drawn(any_entry_visible);
      sec.header_label->set_is_updated(any_entry_visible);
    }
    if (sec.spacer_widget != nullptr) {
      bool show_spacer = any_entry_visible && s > 0;
      sec.spacer_widget->set_is_drawn(show_spacer);
      sec.spacer_widget->set_is_updated(show_spacer);
    }
  }

  m_scroll_view->set_scroll_offset(0);
  m_scroll_view->content()->mark_dirty();
}

void PopupSettings::refresh_save_button_state() {
  if (m_btn_save != nullptr) { m_btn_save->set_disabled(m_working_settings == m_initial_settings); }
}
