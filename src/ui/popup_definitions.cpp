#include "popup_definitions.hpp"
#include <algorithm>
#include <functional>
#include <span>
#include <string>
#include <string_view>
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
  set_layout("ttb expand fit m:8 s:8");

  title = &add_child<zincgui::Label>("");
  title->set_fixed_height(32 * zincbox::ui_scale());
  text_input = &add_child<zincgui::TextInput>();
  text_input->set_fixed_height(zincgui::default_settings::text_input_size.y);

  buttons = &add_child<Widget>();
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);

  btn_cancel = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel->on_press([this]() {
    if (on_cancel_pressed) { on_cancel_pressed(); }
    close();
  });

  btn_ok = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok->on_press([this]() {
    if (on_ok_pressed) { on_ok_pressed(); }
    close();
  });
}

PopupConfirm::PopupConfirm(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_,
                           std::string_view content_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  set_layout("ttb fill fit expand m:8 s:8");

  title = &add_child<zincgui::Label>("");
  title->set_height(32 * zincbox::ui_scale());
  title->set_fixed_height(32 * zincbox::ui_scale());

  content = &add_child<zincgui::Label>(content_);
  content->set_height(content->get_text_extents().y);
  content->set_fixed_height(content->get_text_extents().y);

  buttons = &add_child<Widget>();
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);

  btn_cancel = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel->on_press([this]() {
    if (on_cancel_pressed) { on_cancel_pressed(); }
    close();
  });

  btn_ok = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok->on_press([this]() {
    if (on_ok_pressed) { on_ok_pressed(); }
    close();
  });
}

PopupImportFolders::PopupImportFolders(zincgui::Root& ui_, PopupController& controller_,
                                       std::function<void(Popup*)> on_close_,
                                       std::span<const std::filesystem::path> dropped_directories)
  : Popup(ui_, controller_, std::move(on_close_)) {

  set_layout("ttb expand fit fill m:8 s:8");

  dirs = std::vector<std::filesystem::path>(dropped_directories.begin(), dropped_directories.end());

  title = &add_child<zincgui::Label>("");
  title->set_anchor(zincgui::Anchor::TOP);
  title->set_parent_anchor(zincgui::Anchor::TOP);
  title->set_height(32 * zincbox::ui_scale());
  title->set_fixed_height(32 * zincbox::ui_scale());

  content = &add_child<zincgui::Label>("");
  content->set_anchor(zincgui::Anchor::CENTER);
  content->set_parent_anchor(zincgui::Anchor::CENTER);

  buttons = &add_child<Widget>();
  buttons->set_anchor(zincgui::Anchor::BOTTOM);
  buttons->set_parent_anchor(zincgui::Anchor::BOTTOM);
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);

  btn_cancel = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel->on_press([this]() { close(); });

  std::string content_str;
  i32 index = 1;
  for (const auto& dir : dirs) {
    content_str += std::to_string(index) + ". " + path_to_utf8(dir) + "\n";
    index += 1;
  }
  // remove the last newline
  if (!content_str.empty()) { content_str.pop_back(); }

  content->set_text(content_str);
  content->set_size(content->get_text_extents());
  content->set_fixed_height(content->get_text_extents().y);
  set_width(std::max(content->get_width() + 16, 400));

  if (dirs.size() > 1) {
    auto str_size = std::to_string(dirs.size());
    title->set_text(tr::format("popup.import.title_plural", str_size));

    btn_add_collections =
      &buttons->add_child<zincgui::Button>(tr::format("popup.import.action_add_collections", str_size));
    btn_add_collections->on_press([this]() {
      if (on_add_collections_pressed) { on_add_collections_pressed(dirs); }
      close();
    });

    btn_merge = &buttons->add_child<zincgui::Button>(tr::get("popup.import.action_merge"));
    btn_merge->on_press([this]() {
      if (on_merge_pressed) { on_merge_pressed(dirs); }
      close();
    });
  } else if (dirs.size() == 1) {
    title->set_text(tr::get("popup.import.title_single"));

    btn_add_collections = &buttons->add_child<zincgui::Button>(tr::get("popup.import.action_add_collection"));
    btn_add_collections->on_press([this]() {
      if (on_add_collections_pressed) { on_add_collections_pressed(dirs); }
      close();
    });
  }
}

PopupSetSources::PopupSetSources(zincgui::Root& ui_, PopupController& controller_,
                                 std::function<void(Popup*)> on_close_, size_t collection_id_)
  : Popup(ui_, controller_, std::move(on_close_)), collection_id(collection_id_) {

  set_layout("ttb fill expand m:8 s:8");

  auto& collection = db::collection_by_id(collection_id)->get();

  title = &add_child<zincgui::Label>(tr::format("popup.sources.title", collection.name()));
  title->set_fixed_height(32 * zincbox::ui_scale());
  title->set_anchor(zincgui::Anchor::TOP);
  title->set_parent_anchor(zincgui::Anchor::TOP);

  scrollable_content = &add_child<ScrollableView>();
  scrollable_content->set_clip_children(true);
  scrollable_content->content()->set_layout("ttb s:0 m:0 expand fit");

  buttons = &add_child<Widget>();
  buttons->set_anchor(zincgui::Anchor::BOTTOM);
  buttons->set_parent_anchor(zincgui::Anchor::BOTTOM);
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);

  btn_close = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.close"));
  btn_close->on_press([this]() { close(); });

  btn_add_dir = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.add_directory"));
  btn_add_dir->on_press([this]() {
    if (on_add_dir_pressed) { on_add_dir_pressed(); }
    close();
  });

  if (!collection.paths().empty()) {
    for (i32 num = 1; const auto& path : collection.paths()) {
      std::string str = std::to_string(num) + ". " + path_to_utf8(path);

      auto color_odd = theme::config().panel_tracklist.track_color_odd;
      auto color_even = theme::config().panel_tracklist.track_color_even;
      auto& container =
        scrollable_content->content()->add_child<zincgui::ColorRect>(num % 2 == 0 ? color_odd : color_even);
      container.set_layout("ltr fill expand m:8 s:8");
      container.set_height(40);

      auto& label = container.add_child<zincgui::Label>(str);
      label.set_label_anchor(zincgui::Anchor::LEFT);
      label.set_min_width(0);
      label.set_max_width(0);

      auto& button = container.add_child<zincgui::Button>(tr::get("dialog.action.remove"));
      button.set_min_width(60);
      button.set_max_width(60);
      button.on_press([this, path]() {
        if (on_remove_path_pressed) { on_remove_path_pressed(path); }
      });

      // label.update_text();
      max_path_label_width = std::max<i32>(max_path_label_width, label.get_text_extents().x);

      num += 1;
    }
  } else {
    auto& label = add_child<zincgui::Label>(tr::get("collection.sources.none"));
    label.set_text_color(theme::config().text_color_muted);
    label.set_ignore_parents_layout(true);
    label.set_anchor(zincgui::Anchor::CENTER);
    label.set_parent_anchor(zincgui::Anchor::CENTER);
  }
}

void PopupSetSources::update() {
  set_width(std::min(get_max_content_width(), max_path_label_width * 2));
  set_height(
    std::min(get_max_content_height(), scrollable_content->content()->get_height() + (i32)(150 * zincbox::ui_scale())));

  if (scroll_px != target_scroll_px) {
    double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
    scroll_px = std::lerp(scroll_px, target_scroll_px, t);
    scrollable_content->set_y(-std::floor(scroll_px));
  }

  Popup::update();
}

PopupAddToPlaylist::PopupAddToPlaylist(zincgui::Root& ui_, PopupController& controller_,
                                       std::function<void(Popup*)> on_close_, std::optional<size_t> track_id_)
  : Popup(ui_, controller_, std::move(on_close_)), track_id(track_id_) {

  std::string pretty_track;
  if (track_id.has_value() && db::track_by_id(track_id.value()).has_value()) {
    auto& track = db::track_by_id(track_id.value())->get();
    pretty_track = track.pretty_name();
  }

  set_layout("ttb fill expand m:8 s:8");

  if (!pretty_track.empty()) {
    title = &add_child<zincgui::Label>(tr::format("popup.add_to_playlist.title", pretty_track));
  } else {
    title = &add_child<zincgui::Label>(tr::format("popup.add_to_playlist.title_plural"));
  }
  title->set_fixed_height(32 * zincbox::ui_scale());
  title->set_anchor(zincgui::Anchor::TOP);
  title->set_parent_anchor(zincgui::Anchor::TOP);

  playlists_view = &add_child<PanelPlaylists>();
  playlists_view->props.collection_id = 0;
  playlists_view->props.button_sort_by_visible = false;
  playlists_view->props.button_add_playlist_visible = false;

  playlists_view->on_playlist_lmb = [this](size_t playlist_id, Widget*) {
    if (on_playlist_selected) { on_playlist_selected(playlist_id); }
    close();
  };

  buttons = &add_child<Widget>();
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);
  buttons->set_layout("ltr fill expand m:0 s:8");

  btn_cancel = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel->on_press([this]() { close(); });
}

void PopupAddToPlaylist::update() {
  set_width(get_max_content_width());
  set_height(get_max_content_height());
  Popup::update();
}

PopupCreateSmartPlaylist::PopupCreateSmartPlaylist(zincgui::Root& ui_, PopupController& controller_,
                                                   std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {

  set_layout("ttb fit expand m:8 s:12");
  set_width(400);

  title = &add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.title"));

  Widget& container_name = add_child<Widget>();
  container_name.set_layout("ttb expand m:0 s:8");
  label_name = &container_name.add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.label_input"));
  input_name = &container_name.add_child<zincgui::TextInput>();

  Widget& container_artists = add_child<Widget>();
  container_artists.set_layout("ttb expand m:0 s:8");
  label_artists = &container_artists.add_child<zincgui::Label>(tr::get("popup.smart_playlist.add.label_artists"));
  input_artists = &container_artists.add_child<zincgui::TextInput>();

  buttons = &add_child<Widget>();
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);
  btn_cancel = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.cancel"));
  btn_cancel->on_press([this]() -> void {
    if (on_cancel) { on_cancel(); }
    close();
  });
  btn_add = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.add"));
  btn_add->on_press([this]() -> void {
    if (on_add) { on_add(); }
    close();
  });
}

PopupAbout::PopupAbout(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  set_layout("ttb fit expand m:8 s:12");

  auto& title = add_child<zincgui::Label>(tr::get("popup.about.title"));
  title.set_height(32 * zincbox::ui_scale());
  title.set_resize_to_text_extents(false);
  auto& content =
    add_child<zincgui::Label>(tr::format("popup.about.content", "0.3", __DATE__, "github.com/olekgrzegrzulka/zincbox"));
  content.set_fixed_width(content.get_text_extents().x + 16);
  content.set_fixed_height(content.get_text_extents().y + 16);
  content.set_text_color(theme::config().text_color_muted);
  content.set_text_alignment(zincgui::TextAlign::JUSTIFY);

  auto& buttons = add_child<Widget>();
  buttons.set_layout("ltr fill expand m:0 s:8");
  buttons.set_fixed_height(zincgui::default_settings::button_size.y);
  auto& btn_ok = buttons.add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok.on_press([this]() -> void { close(); });

  set_width(content.get_text_extents().x + 16 + (2 * 8));
}

PopupWelcome::PopupWelcome(zincgui::Root& ui_, PopupController& controller_, std::function<void(Popup*)> on_close_)
  : Popup(ui_, controller_, std::move(on_close_)) {
  set_layout("ttb fit expand m:8 s:12");

  auto& title = add_child<zincgui::Label>(tr::get("popup.welcome.title"));
  title.set_height(32 * zincbox::ui_scale());
  title.set_resize_to_text_extents(false);
  auto& content = add_child<zincgui::Label>(tr::format("popup.welcome.content"));
  content.set_text_color(theme::config().text_color_muted);
  content.set_width(content.get_text_extents().x);

  set_width(content.get_width() + 24);

  auto& buttons = add_child<Widget>();
  buttons.set_layout("ltr fill fit expand m:0 s:8");
  buttons.set_fixed_height(zincgui::default_settings::button_size.y);

  auto& btn_ok = buttons.add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok.on_press([this]() -> void { close(); });
}

PopupScanSummary::PopupScanSummary(zincgui::Root& ui_, PopupController& controller_,
                                   std::function<void(Popup*)> on_close_,
                                   const zincbox::scanner::ImportSummary& summary)
  : Popup(ui_, controller_, std::move(on_close_)) {

  set_layout("ttb fit expand m:12 s:8");

  title = &add_child<zincgui::Label>(tr::get("popup.scan_summary.title"));
  title->set_height(32 * zincbox::ui_scale());
  title->set_min_height(32 * zincbox::ui_scale());
  title->set_anchor(zincgui::Anchor::TOP);
  title->set_parent_anchor(zincgui::Anchor::TOP);

  std::string collection_name;
  if (auto c = db::collection_by_id(summary.collection_id)) { collection_name = c->get().name(); }

  header = &add_child<zincgui::Label>(tr::format("popup.scan_summary.header", collection_name));
  header->set_height(24 * zincbox::ui_scale());
  header->set_text_color(theme::config().text_color_muted);

  scroll_view = &add_child<ScrollableView>();
  scroll_view->content()->set_layout("ttb fit expand mx:6 my:6 s:12");

  auto add_category = [&](std::string_view key, const std::vector<db::track_id_t>& ids) {
    auto& wrapper = scroll_view->content()->add_child<zincgui::Widget>();
    wrapper.set_layout("ttb fit expand m:0 s:4");

    auto& row = wrapper.add_child<zincgui::Widget>();
    row.set_layout("ltr fill expand");
    row.set_height(20 * zincbox::ui_scale());

    auto& label_key = row.add_child<zincgui::Label>(key);
    label_key.set_label_anchor(zincgui::Anchor::LEFT);

    auto& label_val = row.add_child<zincgui::Label>(std::to_string(ids.size()));
    label_val.set_label_anchor(zincgui::Anchor::RIGHT);
    label_val.set_max_width(80 * zincbox::ui_scale());

    if (ids.empty()) {
      label_key.set_text_color(theme::config().text_color_muted);
      label_val.set_text_color(theme::config().text_color_muted);
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
      display_tracks_storage.emplace_back(std::move(ti_vec));

      auto& pt = wrapper.add_child<PanelTracklist>();

      i32 target_panel_h = std::clamp<i32>(ids.size() * 28, 40, 200) * zincbox::ui_scale();
      pt.set_height(target_panel_h);
      pt.set_fixed_height(target_panel_h);
      pt.recreate(display_tracks_storage.back());
    }
  };

  add_category(tr::get("popup.scan_summary.added"), summary.added_tracks);
  add_category(tr::get("popup.scan_summary.changed"), summary.modified_tracks);
  add_category(tr::get("popup.scan_summary.unchanged"), summary.skipped_tracks);
  add_category(tr::get("popup.scan_summary.not_found"), summary.not_found_tracks);

  if (!summary.errors.empty()) {
    auto& wrapper = scroll_view->content()->add_child<zincgui::Widget>();
    wrapper.set_layout("ttb fit expand m:0 s:4");

    auto& row = wrapper.add_child<zincgui::Widget>();
    row.set_layout("ltr fit expand m:0 s:0");
    row.set_height(20 * zincbox::ui_scale());

    auto& label_key = row.add_child<zincgui::Label>(tr::get("popup.scan_summary.errors"));
    label_key.set_label_anchor(zincgui::Anchor::LEFT);
    label_key.set_width(280 * zincbox::ui_scale());
    label_key.set_text_color(vec3f{0.9f, 0.4f, 0.4f});

    auto& label_val = row.add_child<zincgui::Label>(std::to_string(summary.errors.size()));
    label_val.set_label_anchor(zincgui::Anchor::RIGHT);
    label_val.set_width(80 * zincbox::ui_scale());
    label_val.set_text_color(vec3f{0.9f, 0.4f, 0.4f});
  }
  buttons = &add_child<Widget>();
  buttons->set_layout("ltr expand fill m:0 s:8");
  buttons->set_fixed_height(zincgui::default_settings::button_size.y);
  buttons->set_anchor(zincgui::Anchor::BOTTOM);
  buttons->set_parent_anchor(zincgui::Anchor::BOTTOM);

  btn_ok = &buttons->add_child<zincgui::Button>(tr::get("dialog.action.ok"));
  btn_ok->on_press([this]() {
    if (on_ok_pressed) { on_ok_pressed(); }
    close();
  });
}

void PopupScanSummary::update() {
  i32 content_h = scroll_view->content()->get_height();

  i32 max_popup_h = get_max_content_height();

  bool show_headers = ui.get_window_height() > 400;
  title->set_is_drawn(show_headers);
  header->set_is_drawn(show_headers);

  i32 title_h = show_headers ? title->get_height() : 0;
  i32 header_h = show_headers ? header->get_height() : 0;
  i32 non_scroll_h = title_h + header_h + buttons->get_height() + static_cast<i32>(48 * zincbox::ui_scale());

  i32 max_scroll_h = std::max<i32>(20, max_popup_h - non_scroll_h);
  i32 target_scroll_h = std::clamp<i32>(content_h, 20, max_scroll_h);

  scroll_view->set_height(target_scroll_h);
  scroll_view->set_fixed_height(target_scroll_h);

  set_width(std::min<i32>(get_max_content_width(), 550 * zincbox::ui_scale()));

  Popup::update();
}
