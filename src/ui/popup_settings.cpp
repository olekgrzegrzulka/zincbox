#include "popup_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
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
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/spinner.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

namespace {
  i32 scaled(i32 value) { return static_cast<i32>(value * zincbox::ui_scale()); }

  i32 margin_xs() { return scaled(4); }
  i32 margin_sm() { return scaled(6); }
  i32 margin_md() { return scaled(8); }
  i32 margin_lg() { return scaled(12); }

  i32 spacing_xs() { return scaled(4); }
  i32 spacing_sm() { return scaled(6); }
  i32 spacing_md() { return scaled(8); }
  i32 spacing_lg() { return scaled(12); }
  i32 spacing_xl() { return scaled(16); }

  i32 title_height() { return scaled(32); }
  i32 header_height() { return scaled(24); }
  i32 small_label_height() { return scaled(16); }
  i32 sidebar_width() { return scaled(130); }
  i32 eq_bands_height() { return scaled(150); }
  i32 settings_popup_width() { return scaled(800); }
  i32 settings_popup_height() { return scaled(600); }
} // namespace

PopupSettings::PopupSettings(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)), m_initial_settings(zincbox::settings()),
    m_working_settings(zincbox::settings()) {

  layout("ttb expand fill");
  layout().set_margin(margin_md());
  layout().spacing = spacing_md();

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
  width(std::min<i32>(max_content_width(), settings_popup_width()))
    .height(std::min<i32>(max_content_height(), settings_popup_height()));
  // refresh_save_button_state();

  bool tight_ = width() < scaled(700);
  if (tight_ != m_tight) {
    m_tight = tight_;
    for (auto* lbl : m_eq_freq_labels) {
      lbl->is_drawn(!m_tight);
    }
    for (auto* lbl : m_eq_value_labels) {
      lbl->is_drawn(!m_tight);
    }
  }

  Popup::update();
}

i32 PopupSettings::control_row_height() { return zincgui::default_settings::text_input_size.y; }

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
  title.resize_to_text_extents(false).fixed_height(title_height());

  m_search_bar = &add_child<zincgui::TextInput>();
  m_search_bar->on_text_changed([this]() -> void { filter_by_search(); });
}

void PopupSettings::build_body() {
  auto& content = add_child<zincgui::Widget>();
  content.layout("ltr expand fill").weight(1.0f);
  content.layout().spacing = spacing_xl();

  auto& sidebar = content.add_child<zincgui::Sprite>("panel_dark");
  sidebar.layout("ttb expand").fixed_width(sidebar_width());

  m_scroll_view = &content.add_child<ScrollableView>();
  m_scroll_view->weight(1.0f);
  m_scroll_view->background()->texture("panel_dark");
  m_scroll_view->content()->layout("ttb fit expand");
  m_scroll_view->content()->layout().margin = vec2i{margin_lg(), margin_md()};
  m_scroll_view->content()->layout().spacing = spacing_sm();

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
    btn.label().text(localized);
    btn.switch_mode(false);
    btn.on_press([this, i]() -> void {
      select_sidebar_button(i);
      scroll_to_section(i);
    });
    m_page_buttons[i] = &btn;
  }
}

void PopupSettings::build_footer() {
  auto& buttons = add_child<zincgui::Widget>();
  buttons.layout("ltr expand fill").fixed_height(zincgui::default_settings::button_size.y);
  buttons.layout().spacing = spacing_md();

  auto& btn_cancel = buttons.add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel.weight(1.0f);
  btn_cancel.on_press([this]() -> void {
    if (m_on_cancel) { m_on_cancel(); }
    close();
  });

  m_btn_save = &buttons.add_child<zincgui::Button>(tr::get("dialog.action.save"));
  m_btn_save->weight(1.0f);
  m_btn_save->on_press([this]() -> void {
    if (m_working_settings == m_initial_settings) { return; }
    m_working_settings.clamp_values();
    if (m_on_save) { m_on_save(m_working_settings); }
    close();
  });
}

void PopupSettings::begin_section(size_t section_idx) {
  auto* parent = m_scroll_view->content();
  if (section_idx > 0) {
    auto& spacer = parent->add_child<zincgui::Widget>();
    spacer.fixed_height(spacing_lg());
    m_sections[section_idx].spacer_widget = &spacer;
  }

  auto& header = parent->add_child<zincgui::Label>(m_sections[section_idx].localized_title);
  header.resize_to_text_extents(false)
    .fixed_height(header_height())
    .text_color(theme::config().text_color_muted)
    .label_anchor(zincgui::Anchor::LEFT)
    .font_size(zincbox::font_size_lg());
  m_sections[section_idx].header_label = &header;
}

zincgui::ComboBox* PopupSettings::add_row_combobox(size_t section_idx, std::string_view tr_key,
                                                   std::function<void(const std::string&)> on_changed) {
  std::string localized = tr::get(std::string(tr_key));
  auto& row = m_scroll_view->content()->add_child<zincgui::Widget>();
  row.layout("ltr fill expand").fixed_height(control_row_height());
  row.layout().spacing = spacing_md();

  auto& lbl = row.add_child<zincgui::Label>(localized);
  lbl.resize_to_text_extents(false).label_anchor(zincgui::Anchor::LEFT).min_width(1);

  auto& combo = row.add_child<zincgui::ComboBox>();
  combo.fixed_height(0);
  combo.on_item_selected([combo_ = &combo, this, cb = std::move(on_changed)]() -> void {
    cb(combo_->selected_item_id());
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
  row.layout("ltr fill expand").fixed_height(control_row_height());
  row.layout().spacing = spacing_md();

  auto& lbl = row.add_child<zincgui::Label>(localized);
  lbl.resize_to_text_extents(false).label_anchor(zincgui::Anchor::LEFT).min_width(1);

  auto& spinner = row.add_child<zincgui::Spinner>();
  if (!postfix.empty()) { spinner.postfix(std::string(postfix)); }
  spinner.min_value(min_val).max_value(max_val).value(initial_val).fixed_height(0);
  spinner.on_value_changed([spinner_ = &spinner, this, cb = std::move(on_changed)]() -> void {
    cb(spinner_->value());
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
  checkbox.checked(initial_checked).checkbox_anchor(zincgui::Anchor::LEFT).fixed_height(0);
  checkbox.on_value_changed([checkbox_ = &checkbox, this, cb = std::move(on_changed)]() -> void {
    cb(checkbox_->checked());
    refresh_save_button_state();
  });

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
  row.layout("ltr fill expand").fixed_height(control_row_height());
  row.layout().spacing = spacing_md();

  auto& lbl = row.add_child<zincgui::Label>(localized_label);
  lbl.resize_to_text_extents(false).label_anchor(zincgui::Anchor::LEFT).min_width(1);

  auto& btn = row.add_child<zincgui::Button>(localized_btn);
  btn.on_press(std::move(on_clicked)).fixed_height(0);

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
  combo_cover->add_item("album", tr::get("settings.playback.source_album"))
    .add_item("playlist", tr::get("settings.playback.source_playlist"));
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
                   [this](bool chk) -> void { m_working_settings.playback.shuffle_allow_same_album = chk; });

  add_row_checkbox(SEC, "settings.playback.allow_same_artist", m_working_settings.playback.shuffle_allow_same_artist,
                   [this](bool chk) -> void { m_working_settings.playback.shuffle_allow_same_artist = chk; });

  add_row_checkbox(SEC, "settings.playback.restart_on_previous", m_working_settings.playback.restart_on_previous,
                   [this](bool chk) -> void { m_working_settings.playback.restart_on_previous = chk; });

  auto* crossfade = add_row_spinner(SEC, "settings.playback.crossfade", 0, 5000, m_working_settings.playback.crossfade,
                                    "ms", [this](i32 val) -> void { m_working_settings.playback.crossfade = val; });
  crossfade->button_step(25);

  build_equalizer_widget(SEC);
}

void PopupSettings::build_equalizer_widget(size_t section_idx) {
  std::string eq_title_loc = tr::get("settings.playback.equalizer");
  std::string eq_reset_loc = tr::get("settings.playback.equalizer_reset");

  auto& eq_wrapper = m_scroll_view->content()->add_child<zincgui::Widget>();
  eq_wrapper.layout("ttb fit expand");
  eq_wrapper.layout().spacing = spacing_sm();

  auto& header_row = eq_wrapper.add_child<zincgui::Widget>();
  header_row.layout("ltr fill expand").fixed_height(control_row_height());
  header_row.layout().spacing = spacing_md();

  auto& eq_label = header_row.add_child<zincgui::Label>(eq_title_loc);
  eq_label.resize_to_text_extents(false).label_anchor(zincgui::Anchor::LEFT).min_width(1);

  auto& reset_btn = header_row.add_child<zincgui::Button>(eq_reset_loc);
  reset_btn.fixed_height(control_row_height());
  reset_btn.on_press([this]() -> void {
    for (size_t i = 0; i < player::EQ_BAND_COUNT; i += 1) {
      m_working_settings.playback.equalizer[i] = 0.0f;
      m_eq_sliders[i]->value(0.0f, false);
      m_eq_value_labels[i]->text(format_eq_db(0.0f));
    }
    refresh_save_button_state();
  });

  auto& bands_row = eq_wrapper.add_child<zincgui::Widget>();
  bands_row.layout("ltr fill expand").fixed_height(eq_bands_height());
  bands_row.layout().set_margin(margin_xs());
  bands_row.layout().spacing = spacing_xs();

  for (size_t i = 0; i < player::EQ_BAND_COUNT; i += 1) {
    auto& col = bands_row.add_child<zincgui::Widget>();
    col.layout("ttb fill expand");
    col.layout().spacing = spacing_xs();

    float init_gain = std::clamp(m_working_settings.playback.equalizer[i], -12.0f, 12.0f);

    auto& val_label = col.add_child<zincgui::Label>(format_eq_db(init_gain));
    val_label.resize_to_text_extents(false).fixed_height(small_label_height()).label_anchor(zincgui::Anchor::CENTER);
    m_eq_value_labels[i] = &val_label;

    auto& slider = col.add_child<ZincboxSlider>("equalizer");
    slider.orientation(zincgui::SliderOrientation::VERTICAL_INVERTED)
      .fixed_width(slider.width())
      .track_nine_slice_margin(3)
      .thumb_nine_slice_margin(6)
      .min_value(-12.0f)
      .max_value(12.0f)
      .sensitivity(1.0f)
      .value(init_gain, false)
      .is_scrollable(false);
    slider.on_value_changed([this, i](float, float new_val) -> void {
      m_working_settings.playback.equalizer[i] = std::clamp(new_val, -12.0f, 12.0f);
      m_eq_value_labels[i]->text(format_eq_db(m_working_settings.playback.equalizer[i]));
      refresh_save_button_state();
    });
    m_eq_sliders[i] = &slider;

    auto& freq_label = col.add_child<zincgui::Label>(format_eq_freq(player::EQ_FREQUENCIES[i]));
    freq_label.resize_to_text_extents(false)
      .fixed_height(small_label_height())
      .label_anchor(zincgui::Anchor::CENTER)
      .text_color(theme::config().text_color_muted);
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
    m_page_buttons[j]->is_switched(j == active_idx);
  }
}

void PopupSettings::scroll_to_section(size_t section_idx) {
  if (section_idx >= SECTION_COUNT) { return; }
  auto* header = m_sections[section_idx].header_label;
  if (header != nullptr && header->is_drawn()) { m_scroll_view->scroll_offset(header->y()); }
}

void PopupSettings::filter_by_search() {
  std::string query = sanitize_query(m_search_bar->label().text());
  if (query == m_last_search_query) { return; }
  m_last_search_query = query;

  for (size_t s = 0; s < SECTION_COUNT; s += 1) {
    auto& sec = m_sections[s];
    bool section_title_matches = !query.empty() && sec.search_blob.find(query) != std::string::npos;
    bool any_entry_visible = false;

    for (auto& entry : sec.entries) {
      bool visible = query.empty() || section_title_matches || entry.search_blob.find(query) != std::string::npos;
      entry.row_widget->is_drawn(visible).is_updated(visible);
      if (visible) { any_entry_visible = true; }
    }

    if (sec.header_label != nullptr) { sec.header_label->is_drawn(any_entry_visible).is_updated(any_entry_visible); }
    if (sec.spacer_widget != nullptr) {
      bool show_spacer = any_entry_visible && s > 0;
      sec.spacer_widget->is_drawn(show_spacer).is_updated(show_spacer);
    }
  }

  m_scroll_view->scroll_offset(0);
  m_scroll_view->content()->mark_dirty();
}

void PopupSettings::refresh_save_button_state() {
  if (m_btn_save != nullptr) { m_btn_save->disabled(m_working_settings == m_initial_settings); }
}
