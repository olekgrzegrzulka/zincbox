#include "popup_definitions.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/scanner.hpp"
#include "ui/panel_playlists.hpp"
#include "ui/panel_tracklist.hpp"
#include "ui/popup.hpp"
#include "ui/popup_controller.hpp"
#include "ui/scrollable_view.hpp"
#include "ui/theme.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

PopupInput::PopupInput(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  layout("ttb expand fit m:8 s:8");

  m_title = &add_child<zincgui::Label>("");
  m_title->fixed_height(32 * zincbox::ui_scale());
  m_text_input = &add_child<zincgui::TextInput>();
  m_text_input->fixed_height(zincgui::default_settings::text_input_size.y);

  m_buttons = &add_child<Widget>();
  m_buttons->layout("ltr expand fill m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);

  m_btn_cancel = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  m_btn_cancel->on_press([this]() {
    if (m_on_cancel_pressed) { m_on_cancel_pressed(); }
    close();
  });

  m_btn_ok = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  m_btn_ok->on_press([this]() {
    if (m_on_ok_pressed) { m_on_ok_pressed(); }
    close();
  });
}

PopupConfirm::PopupConfirm(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                           std::string_view content_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  layout("ttb fill fit expand m:8 s:8");

  m_title = &add_child<zincgui::Label>("");
  m_title->height(32 * zincbox::ui_scale()).fixed_height(32 * zincbox::ui_scale());

  m_content = &add_child<zincgui::Label>(content_);
  m_content->height(m_content->text_extents().y).fixed_height(m_content->text_extents().y);

  m_buttons = &add_child<Widget>();
  m_buttons->layout("ltr expand fill m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);

  m_btn_cancel = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  m_btn_cancel->on_press([this]() {
    if (m_on_cancel_pressed) { m_on_cancel_pressed(); }
    close();
  });

  m_btn_ok = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  m_btn_ok->on_press([this]() {
    if (m_on_ok_pressed) { m_on_ok_pressed(); }
    close();
  });
}

PopupImportFolders::PopupImportFolders(zincgui::Root& ui_, PopupController& controller_,
                                       std::function<void(Popup*)> on_close_,
                                       std::span<const std::filesystem::path> dropped_directories)
  : Popup(ui_, controller_, std::move(on_close_)) {

  layout("ttb expand fit fill m:8 s:8");

  m_dirs = std::vector<std::filesystem::path>(dropped_directories.begin(), dropped_directories.end());

  m_title = &add_child<zincgui::Label>("");
  m_title->anchor(zincgui::Anchor::TOP)
    .parent_anchor(zincgui::Anchor::TOP)
    .height(32 * zincbox::ui_scale())
    .fixed_height(32 * zincbox::ui_scale());

  m_content = &add_child<zincgui::Label>("");
  m_content->anchor(zincgui::Anchor::CENTER).parent_anchor(zincgui::Anchor::CENTER);

  m_buttons = &add_child<Widget>();
  m_buttons->anchor(zincgui::Anchor::BOTTOM)
    .parent_anchor(zincgui::Anchor::BOTTOM)
    .layout("ltr expand fill m:0 s:8")
    .fixed_height(zincgui::default_settings::button_size.y);

  m_btn_cancel = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  m_btn_cancel->on_press([this]() { close(); });

  std::string content_str;
  i32 index = 1;
  for (const auto& dir : m_dirs) {
    content_str += std::to_string(index) + ". " + path_to_utf8(dir) + "\n";
    index += 1;
  }
  // remove the last newline
  if (!content_str.empty()) { content_str.pop_back(); }

  m_content->text(content_str).size(m_content->text_extents()).fixed_height(m_content->text_extents().y);
  width(std::max(m_content->width() + 16, 400));

  if (m_dirs.size() > 1) {
    auto str_size = std::to_string(m_dirs.size());
    m_title->text(tr::format("popup.import.title_plural", str_size));

    m_btn_add_collections =
      &m_buttons->add_child<zincgui::Button>(tr::format("popup.import.action_add_collections", str_size));
    m_btn_add_collections->on_press([this]() {
      if (m_on_add_collections_pressed) { m_on_add_collections_pressed(m_dirs); }
      close();
    });

    m_btn_merge = &m_buttons->add_child<zincgui::Button>(tr::get("popup.import.action_merge"));
    m_btn_merge->on_press([this]() {
      if (m_on_merge_pressed) { m_on_merge_pressed(m_dirs); }
      close();
    });
  } else if (m_dirs.size() == 1) {
    m_title->text(tr::get("popup.import.title_single"));

    m_btn_add_collections = &m_buttons->add_child<zincgui::Button>(tr::get("popup.import.action_add_collection"));
    m_btn_add_collections->on_press([this]() {
      if (m_on_add_collections_pressed) { m_on_add_collections_pressed(m_dirs); }
      close();
    });
  }
}

PopupSetSources::PopupSetSources(zincgui::Root& ui_, PopupController& controller_,
                                 std::function<void(Popup*)> on_close_, size_t collection_id_)
  : Popup(ui_, controller_, std::move(on_close_)), m_collection_id(collection_id_) {

  layout("ttb fill expand m:8 s:8");

  auto& collection = db::collection_by_id(m_collection_id)->get();

  m_title = &add_child<zincgui::Label>(tr::format("popup.sources.title", collection.name()));
  m_title->fixed_height(32 * zincbox::ui_scale()).anchor(zincgui::Anchor::TOP).parent_anchor(zincgui::Anchor::TOP);

  m_scrollable_content = &add_child<ScrollableView>();
  m_scrollable_content->clip_children(true);
  m_scrollable_content->content()->layout("ttb s:0 m:0 expand fit");

  m_buttons = &add_child<Widget>();
  m_buttons->anchor(zincgui::Anchor::BOTTOM)
    .parent_anchor(zincgui::Anchor::BOTTOM)
    .layout("ltr expand fill m:0 s:8")
    .fixed_height(zincgui::default_settings::button_size.y);

  m_btn_close = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.close"));
  m_btn_close->on_press([this]() { close(); });

  m_btn_add_dir = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.add_directory"));
  m_btn_add_dir->on_press([this]() {
    if (m_on_add_dir_pressed) { m_on_add_dir_pressed(); }
    close();
  });

  if (!collection.paths().empty()) {
    for (i32 num = 1; const auto& path : collection.paths()) {
      std::string str = std::to_string(num) + ". " + path_to_utf8(path);

      auto color_odd = theme::config().panel_tracklist.track_color_odd;
      auto color_even = theme::config().panel_tracklist.track_color_even;
      auto& container =
        m_scrollable_content->content()->add_child<zincgui::ColorRect>(num % 2 == 0 ? color_odd : color_even);
      container.layout("ltr fill expand m:8 s:8").height(40);

      auto& label = container.add_child<zincgui::Label>(str);
      label.label_anchor(zincgui::Anchor::LEFT).min_width(0).max_width(0);

      auto& button = container.add_child<zincgui::Button>(tr::get("dialog.action.remove"));
      button.min_width(60).max_width(60);
      button.on_press([this, path]() {
        if (m_on_remove_path_pressed) { m_on_remove_path_pressed(path); }
      });

      m_max_path_label_width = std::max<i32>(m_max_path_label_width, label.text_extents().x);

      num += 1;
    }
  } else {
    auto& label = add_child<zincgui::Label>(tr::get("collection.sources.none"));
    label.text_color(theme::config().text_color_muted)
      .ignore_parents_layout(true)
      .anchor(zincgui::Anchor::CENTER)
      .parent_anchor(zincgui::Anchor::CENTER);
  }
}

void PopupSetSources::update() {
  width(std::min(max_content_width(), m_max_path_label_width * 2));
  height(std::min(max_content_height(), m_scrollable_content->content()->height() + (i32)(150 * zincbox::ui_scale())));

  if (m_scroll_px != m_target_scroll_px) {
    double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
    m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);
    m_scrollable_content->y(-std::floor(m_scroll_px));
  }

  Popup::update();
}

PopupAddToPlaylist::PopupAddToPlaylist(zincgui::Root& ui_, PopupController& controller_,
                                       std::function<void(Popup*)> on_close_, std::optional<size_t> track_id_)
  : Popup(ui_, controller_, std::move(on_close_)), m_track_id(track_id_) {

  std::string pretty_track;
  if (m_track_id.has_value() && db::track_by_id(m_track_id.value()).has_value()) {
    auto& track = db::track_by_id(m_track_id.value())->get();
    pretty_track = track.pretty_name();
  }

  layout("ttb fill expand m:8 s:8");

  if (!pretty_track.empty()) {
    m_title = &add_child<zincgui::Label>(tr::format("popup.add_to_playlist.title", pretty_track));
  } else {
    m_title = &add_child<zincgui::Label>(tr::format("popup.add_to_playlist.title_plural"));
  }
  m_title->fixed_height(32 * zincbox::ui_scale()).anchor(zincgui::Anchor::TOP).parent_anchor(zincgui::Anchor::TOP);

  m_playlists_view = &add_child<PanelPlaylists>();
  m_playlists_view->props().collection_id = 0;
  m_playlists_view->props().button_sort_by_visible = false;
  m_playlists_view->props().button_add_playlist_visible = false;

  m_playlists_view->on_playlist_lmb([this](size_t pl_id, Widget*) {
    if (m_on_playlist_selected) { m_on_playlist_selected(pl_id); }
    close();
  });

  m_buttons = &add_child<Widget>();
  m_buttons->fixed_height(zincgui::default_settings::button_size.y).layout("ltr fill expand m:0 s:8");

  m_btn_cancel = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  m_btn_cancel->on_press([this]() { close(); });
}

void PopupAddToPlaylist::update() {
  width(max_content_width()).height(max_content_height());
  Popup::update();
}

PopupCreateSmartPlaylist::PopupCreateSmartPlaylist(zincgui::Root& ui_, PopupController& controller_,
                                                   std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {

  layout("ttb fit expand m:8 s:12").width(400);

  m_title = &add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.title"));

  Widget& container_name = add_child<Widget>();
  container_name.layout("ttb expand m:0 s:8");
  m_label_name = &container_name.add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.label_input"));
  m_input_name = &container_name.add_child<zincgui::TextInput>();

  Widget& container_artists = add_child<Widget>();
  container_artists.layout("ttb expand m:0 s:8");
  m_label_artists = &container_artists.add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.label_artists"));
  m_input_artists = &container_artists.add_child<zincgui::TextInput>();

  m_buttons = &add_child<Widget>();
  m_buttons->layout("ltr expand fill m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);
  m_btn_cancel = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  m_btn_cancel->on_press([this]() -> void {
    if (m_on_cancel) { m_on_cancel(); }
    close();
  });
  m_btn_add = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.add"));
  m_btn_add->on_press([this]() -> void {
    if (m_on_add) { m_on_add(); }
    close();
  });
}

PopupAbout::PopupAbout(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  layout("ttb fit expand m:8 s:12");

  auto& title = add_child<zincgui::Label>(tr::get("popup.about.title"));
  title.height(32 * zincbox::ui_scale()).resize_to_text_extents(false);
  auto& content =
    add_child<zincgui::Label>(tr::format("popup.about.content", "0.3", __DATE__, "github.com/olekgrzegrzulka/zincbox"));
  content.fixed_width(content.text_extents().x + 16)
    .fixed_height(content.text_extents().y + 16)
    .text_color(theme::config().text_color_muted)
    .text_alignment(zincgui::TextAlign::JUSTIFY);

  auto& buttons = add_child<Widget>();
  buttons.layout("ltr fill expand m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);
  auto& btn_ok = buttons.add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok.on_press([this]() -> void { close(); });

  width(content.text_extents().x + 16 + (2 * 8));
}

PopupWelcome::PopupWelcome(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  layout("ttb fit expand m:8 s:12");

  auto& title = add_child<zincgui::Label>(tr::get("popup.welcome.title"));
  title.height(32 * zincbox::ui_scale()).resize_to_text_extents(false);
  auto& content = add_child<zincgui::Label>(tr::format("popup.welcome.content"));
  content.text_color(theme::config().text_color_muted).width(content.text_extents().x);

  width(content.width() + 24);

  auto& buttons = add_child<Widget>();
  buttons.layout("ltr fill fit expand m:0 s:8").fixed_height(zincgui::default_settings::button_size.y);

  auto& btn_ok = buttons.add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok.on_press([this]() -> void { close(); });
}

PopupScanSummary::PopupScanSummary(zincgui::Root& ui_, PopupController& controller_,
                                   std::function<void(Popup*)> on_close_,
                                   const zincbox::scanner::ImportSummary& summary)
  : Popup(ui_, controller_, std::move(on_close_)) {

  layout("ttb fit expand m:12 s:8");

  m_title = &add_child<zincgui::Label>(tr::get("popup.scan_summary.title"));
  m_title->height(32 * zincbox::ui_scale())
    .min_height(32 * zincbox::ui_scale())
    .anchor(zincgui::Anchor::TOP)
    .parent_anchor(zincgui::Anchor::TOP);

  std::string collection_name;
  if (auto c = db::collection_by_id(summary.collection_id)) { collection_name = c->get().name(); }

  m_header = &add_child<zincgui::Label>(tr::format("popup.scan_summary.header", collection_name));
  m_header->height(24 * zincbox::ui_scale()).text_color(theme::config().text_color_muted);

  m_scroll_view = &add_child<ScrollableView>();
  m_scroll_view->content()->layout("ttb fit expand mx:6 my:6 s:12");

  auto add_category = [&](std::string_view key, const std::vector<db::track_id_t>& ids) {
    auto& wrapper = m_scroll_view->content()->add_child<zincgui::Widget>();
    wrapper.layout("ttb fit expand m:0 s:4");

    auto& row = wrapper.add_child<zincgui::Widget>();
    row.layout("ltr fill expand").height(20 * zincbox::ui_scale());

    auto& label_key = row.add_child<zincgui::Label>(key);
    label_key.label_anchor(zincgui::Anchor::LEFT);

    auto& label_val = row.add_child<zincgui::Label>(std::to_string(ids.size()));
    label_val.label_anchor(zincgui::Anchor::RIGHT).max_width(80 * zincbox::ui_scale());

    if (ids.empty()) {
      label_key.text_color(theme::config().text_color_muted);
      label_val.text_color(theme::config().text_color_muted);
    } else {
      std::vector<db::track_info> ti_vec;
      ti_vec.reserve(ids.size());
      size_t idx = 1;
      for (auto id : ids) {
        db::track_info ti;
        ti.track_id = id;
        ti.index = idx++;
        ti_vec.emplace_back(ti);
      }
      m_display_tracks_storage.emplace_back(std::move(ti_vec));

      auto& pt = wrapper.add_child<PanelTracklist>();

      i32 target_panel_h = std::clamp<i32>(ids.size() * 28, 40, 200) * zincbox::ui_scale();
      pt.height(target_panel_h).fixed_height(target_panel_h);
      pt.recreate(m_display_tracks_storage.back());
    }
  };

  add_category(tr::get("popup.scan_summary.added"), summary.added_tracks);
  add_category(tr::get("popup.scan_summary.changed"), summary.modified_tracks);
  add_category(tr::get("popup.scan_summary.unchanged"), summary.skipped_tracks);
  add_category(tr::get("popup.scan_summary.not_found"), summary.not_found_tracks);

  if (!summary.errors.empty()) {
    auto& wrapper = m_scroll_view->content()->add_child<zincgui::Widget>();
    wrapper.layout("ttb fit expand m:0 s:4");

    auto& row = wrapper.add_child<zincgui::Widget>();
    row.layout("ltr fit expand m:0 s:0").height(20 * zincbox::ui_scale());

    auto& label_key = row.add_child<zincgui::Label>(tr::get("popup.scan_summary.errors"));
    label_key.label_anchor(zincgui::Anchor::LEFT).width(280 * zincbox::ui_scale()).text_color(vec3f{0.9f, 0.4f, 0.4f});

    auto& label_val = row.add_child<zincgui::Label>(std::to_string(summary.errors.size()));
    label_val.label_anchor(zincgui::Anchor::RIGHT).width(80 * zincbox::ui_scale()).text_color(vec3f{0.9f, 0.4f, 0.4f});
  }
  m_buttons = &add_child<Widget>();
  m_buttons->layout("ltr expand fill m:0 s:8")
    .fixed_height(zincgui::default_settings::button_size.y)
    .anchor(zincgui::Anchor::BOTTOM)
    .parent_anchor(zincgui::Anchor::BOTTOM);

  m_btn_ok = &m_buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  m_btn_ok->on_press([this]() {
    if (m_on_ok_pressed) { m_on_ok_pressed(); }
    close();
  });
}

void PopupScanSummary::update() {
  i32 content_h = m_scroll_view->content()->height();

  i32 max_popup_h = max_content_height();

  bool show_headers = ui().get_window_height() > 400;
  m_title->is_drawn(show_headers);
  m_header->is_drawn(show_headers);

  i32 title_h = show_headers ? m_title->height() : 0;
  i32 header_h = show_headers ? m_header->height() : 0;
  i32 non_scroll_h = title_h + header_h + m_buttons->height() + static_cast<i32>(48 * zincbox::ui_scale());

  i32 max_scroll_h = std::max<i32>(20, max_popup_h - non_scroll_h);
  i32 target_scroll_h = std::clamp<i32>(content_h, 20, max_scroll_h);

  m_scroll_view->height(target_scroll_h).fixed_height(target_scroll_h);

  width(std::min<i32>(max_content_width(), 550 * zincbox::ui_scale()));

  Popup::update();
}
