#include <array>
#include <filesystem>
#include <functional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmrc/cmrc.hpp>
#include <glaze/glaze.hpp>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/serialized_state.hpp" // for glz::meta<rgba> specialization
#include "common/types.hpp"
#include "common/utf.hpp"
#include "core/io.hpp"
#include "core/settings.hpp"
#include "lib/miniz/miniz.h"
#include "stb_image.h"
#include "theme.hpp"
#include "theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/ui.hpp"
#include "zincbox.hpp"

namespace fs = std::filesystem;
using namespace zincgui;

CMRC_DECLARE(zincbox_resources);

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

static std::unordered_map<std::string, std::vector<uint8_t>, StringHash, std::equal_to<>> resources;
static std::unordered_map<std::string, std::vector<uint8_t>, StringHash, std::equal_to<>> languages;
static std::string resources_ttf_path;
static ThemeConfig config_;

static const std::vector<uint8_t> NO_RESOURCE = {};

const std::vector<uint8_t>& theme::get_raw_resource(const std::string& path) {
  auto it = resources.find(path);
  if (it == resources.end()) {
    return NO_RESOURCE;
  } else {
    return it->second;
  }
}

const ThemeConfig& theme::config() { return config_; }

void load_resources() {
  if (!resources.empty()) { return; }

  resources.clear();
  resources_ttf_path.clear();

  auto cmrc_fs = cmrc::zincbox_resources::get_filesystem();

  if (!cmrc_fs.exists("theme.zip")) {
    out::critical("theme.zip not found in cmrc");
    exit(1);
  }

  {
    auto theme_zip = cmrc_fs.open("theme.zip");
    mz_zip_archive zip_archive{};
    if (!mz_zip_reader_init_mem(&zip_archive, theme_zip.begin(), theme_zip.size(), 0)) {
      out::critical("failed to load resources from memory");
      exit(1);
    }

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) { continue; }
      if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) { continue; }

      std::vector<uint8_t> buffer(file_stat.m_uncomp_size);
      mz_zip_reader_extract_to_mem(&zip_archive, i, buffer.data(), buffer.size(), 0);

      std::string filename(file_stat.m_filename);

      if (resources_ttf_path.empty() && filename.ends_with(".ttf")) { resources_ttf_path = file_stat.m_filename; }

      resources[std::move(filename)] = std::move(buffer);
    }
    mz_zip_reader_end(&zip_archive);
  }

  {
    auto lang_zip = cmrc_fs.open("lang.zip");
    mz_zip_archive zip_archive{};
    if (!mz_zip_reader_init_mem(&zip_archive, lang_zip.begin(), lang_zip.size(), 0)) {
      out::critical("failed to load languages from memory");
      exit(1);
    }

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++) {
      mz_zip_archive_file_stat file_stat;
      if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) { continue; }
      if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) { continue; }

      std::vector<uint8_t> buffer(file_stat.m_uncomp_size);
      mz_zip_reader_extract_to_mem(&zip_archive, i, buffer.data(), buffer.size(), 0);

      std::string filename(file_stat.m_filename);

      if (filename.ends_with(".json")) {
        fs::path file_path{file_stat.m_filename};
        languages[file_path.stem().c_str()] = std::move(buffer);
      }
    }
    mz_zip_reader_end(&zip_archive);
  }
}

std::set<std::string> theme::get_themes() {
  std::set<std::string> ret;

  for (const auto& entry : fs::directory_iterator(io::get_themes_path())) {
    if (fs::is_regular_file(entry.path() / "theme.json")) {
      const std::string name = path_to_utf8(entry.path().filename());
      ret.insert(name);
    }
  }

  return ret;
}

std::set<std::string> theme::get_languages() {
  load_resources();
  std::set<std::string> ret;
  for (auto& [lang, _] : languages) {
    ret.emplace(lang);
  }
  return ret;
}

static bool load_language_from_resource(std::string_view language) {
  load_resources();
  auto it = languages.find(language);
  if (it == resources.end()) { return false; }

  std::string content(reinterpret_cast<const char*>(it->second.data()), it->second.size());
  return tr::load_from_string(content);
}

static bool load_language_from_theme_path(const fs::path& theme_path, std::string_view language) {
  fs::path file_path = theme_path / "lang" / (std::string(language) + ".json");
  if (!fs::is_regular_file(file_path)) { return false; }
  return tr::load_from_file(file_path);
}

static void load_translations(std::string_view theme_name, std::string_view language) {
  bool loaded = false;
  if (theme_name != "") {
    fs::path theme_path(io::get_themes_path() / theme_name);
    loaded = load_language_from_theme_path(theme_path, language);
  }

  if (!loaded) { loaded = load_language_from_resource(language); }
  if (!loaded && language != "en-US") { loaded = load_language_from_resource("en-US"); }
  if (!loaded) {
    out::critical("failed to load translations for language {}", std::string(language));
    exit(1);
  }
}

void theme::load_default_theme(Root& ui, std::string_view language) { load_theme("", ui, language); }

void theme::load_theme(std::string_view theme_name, Root& ui, std::string_view language) {
  const bool load_theme_from_resources = theme_name == "";
  if (load_theme_from_resources) { load_resources(); }

  fs::path theme_path(io::get_themes_path() / theme_name);

  if (!load_theme_from_resources) {
    // Check if the theme exists in the themes directory
    if (!fs::is_directory(theme_path)) {
      out::warn("no theme found at {}", path_to_utf8(theme_path));
      load_theme("", ui, language);
      return;
    }

    fs::path theme_json_path = io::get_themes_path() / theme_name / "theme.json";
    auto error = glz::read_file_json(config_, path_to_utf8(theme_json_path).c_str(), std::string{});
    if (error) {
      out::warn("failed to load theme {} invalid or missing theme.json", theme_name);
      load_theme("", ui, language);
      return;
    }

    // Try to load a font file, else fallback to default theme
    std::string font_path = "";
    for (auto const& dir_entry : fs::recursive_directory_iterator(theme_path)) {
      if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".ttf") {
        font_path = path_to_utf8(dir_entry.path());
        break;
      }
    }
    if (font_path != "") {
      ui.set_font_face(font_path, zincbox::settings().interface.font_size);
    } else {
      out::warn("no ttf file found in {}", std::string{theme_name});
      load_theme("", ui, language);
      return;
    }
  } else {
    load_resources();
    if (!resources.contains("theme.json")) {
      out::critical("theme.json not found");
      exit(1);
    }

    auto error = glz::read_json(config_, reinterpret_cast<const char*>(resources["theme.json"].data()));
    if (error) {
      out::critical("failed to parse theme.json");
      exit(1);
    }

    if (resources_ttf_path.empty()) {
      out::critical("no ttf file found in default theme");
      exit(1);
    }
    ui.set_font_face_from_data(resources[resources_ttf_path].data(), resources[resources_ttf_path].size(),
                               zincbox::settings().interface.font_size);
  }

  load_translations(theme_name, language);

  ScopeTimer timer("load_theme_atlas");

  auto& atlas = ui.get_texture_atlas();

  // please based on this git diff adjust this code block. then tell me what id's i have to internally change

  auto atlas_add_texture = [&load_theme_from_resources, &theme_path,
                            &atlas](const std::string& id, std::vector<std::string> paths = {}) -> bool {
    if (paths.empty()) { return false; }

    if (!load_theme_from_resources) {
      for (const std::string& path : paths) {
        if (fs::is_regular_file((theme_path / path))) {
          atlas.add_texture(id, path_to_utf8(theme_path / path));
          return true;
        }
      }
    }

    out::debug_warn("theme has no {}, loading from default theme", paths[0]);
    load_resources();
    for (const std::string& path : paths) {
      auto it = resources.find(path);
      if (it == resources.end()) { continue; }

      i32 w, h, channels;
      u8* img = stbi_load_from_memory(it->second.data(), it->second.size(), &w, &h, &channels, STBI_rgb_alpha);
      if (!img) { continue; }
      atlas.add_texture(id, img, w, h);
      stbi_image_free(img);

      return true;
    }
    return false;
  };

  auto atlas_add_texture_row = [&](std::span<const std::string> ids, std::span<const std::string> paths = {}) -> bool {
    if (paths.empty()) { return false; }

    if (!load_theme_from_resources) {
      for (const std::string& path : paths) {
        if (fs::is_regular_file((theme_path / path))) {
          atlas.add_texture_row(ids, path_to_utf8(theme_path / path));
          return true;
        }
      }
    }

    out::debug_warn("theme has no {}, loading from default theme", paths[0]);
    load_resources();
    for (const std::string& path : paths) {
      auto it = resources.find(path);
      if (it == resources.end()) { continue; }

      i32 w, h, channels;
      u8* img = stbi_load_from_memory(it->second.data(), it->second.size(), &w, &h, &channels, STBI_rgb_alpha);
      if (!img) { continue; }

      atlas.add_texture_row(ids, img, w, h);
      stbi_image_free(img);
      return true;
    }
    return false;
  };

  using namespace std::string_literals;

  auto add_custom_button = [&](const std::string& id, const std::string& path) {
    std::array<std::string, 4> ids = {id + "_idle", id + "_hovered", id + "_pressed", id + "_disabled"};

    std::vector<std::string> paths = {path, "ui/button.png"};

    if (!atlas_add_texture_row(ids, paths)) {
      atlas_add_texture(id + "_disabled", {id + "_disabled.png", id + ".png", "ui/button_disabled.png"});
      atlas_add_texture(id + "_hovered", {id + "_hovered.png", id + ".png", "ui/button_hovered.png"});
      atlas_add_texture(id + "_idle", {id + "_idle.png", id + ".png", "ui/button_idle.png"});
      atlas_add_texture(id + "_pressed", {id + "_pressed.png", id + ".png", "ui/button_pressed.png"});
    }
  };

  auto add_custom_slider = [&](const std::string& name, const std::string& path_track,
                               const std::string& path_thumb) -> void {
    std::array<std::string, 3> thumb_ids = {name + "_thumb_idle", name + "_thumb_hovered", name + "_thumb_pressed"};
    std::array<std::string, 2> track_ids = {name + "_track_inactive", name + "_track_active"};

    std::vector<std::string> paths_track = {path_track, "ui/slider_track.png"};
    std::vector<std::string> paths_thumb = {path_thumb, "ui/slider_thumb.png"};

    atlas_add_texture_row(thumb_ids, paths_thumb);
    atlas_add_texture_row(track_ids, paths_track);
  };

  auto add_custom_panel = [&](const std::string& id, const std::string& path) -> void {
    std::vector<std::string> paths = {path, "ui/panel.png"};
    atlas_add_texture(id, paths);
  };

  atlas_add_texture("cover_unknown", {"cover_unknown.png"});
  // ui
  atlas_add_texture_row(std::array{"button_idle"s, "button_hovered"s, "button_pressed"s, "button_disabled"s},
                        std::array{"ui/button.png"s});
  atlas_add_texture("panel", {"ui/panel.png"});
  atlas_add_texture("panel_dark", {"ui/panel_dark.png"});
  add_custom_panel("panel_popup", "ui/panel_popup.png");
  add_custom_button("combobox", "ui/combobox.png");
  atlas_add_texture("combobox_contract", {"ui/combobox_contract.png"});
  atlas_add_texture("combobox_expand", {"ui/combobox_expand.png"});
  add_custom_panel("panel_combobox", "ui/panel_combobox.png");
  add_custom_button("combobox_item", "ui/combobox_item.png");
  add_custom_slider("slider", "ui/slider_track.png", "ui/slider_thumb.png");
  add_custom_slider("scrollbar", "ui/scrollbar_track.png", "ui/scrollbar_thumb.png");
  add_custom_slider("volume_bar", "ui/volume_bar_track.png", "ui/volume_bar_thumb.png");
  add_custom_button("spinner_decrease", "ui/spinner_decrease.png");
  add_custom_button("spinner_increase", "ui/spinner_increase.png");
  atlas_add_texture("text_input_focused", {"ui/text_input_focused.png"});
  atlas_add_texture("text_input_idle", {"ui/text_input_idle.png"});
  atlas_add_texture("tooltip", {"ui/tooltip.png"});
  add_custom_button("checkbox", "ui/checkbox.png");
  atlas_add_texture("checkbox_check", {"ui/checkbox_check.png"});
  atlas_add_texture_row(
    std::array{"tab_active_idle"s, "tab_active_hovered"s, "tab_active_pressed"s, "tab_active_disabled"s},
    std::array{"ui/tab_active.png"s});
  atlas_add_texture_row(
    std::array{"tab_inactive_idle"s, "tab_inactive_hovered"s, "tab_inactive_pressed"s, "tab_inactive_disabled"s},
    std::array{"ui/tab_inactive.png"s});
  atlas_add_texture("popover_panel", {"ui/popover_panel.png"});
  atlas_add_texture("popover_arrow", {"ui/popover_arrow.png"});
  atlas_add_texture("popover_arrow_inverted", {"ui/popover_arrow_inverted.png"});
  add_custom_button("button_popover", "ui/button_popover.png");
  atlas_add_texture("notification", {"ui/notification.png"});
  atlas_add_texture("notification_error", {"ui/notification_error.png"});

  // panel_controls
  add_custom_button("play_pause", "panel_controls/button_play_pause.png");
  add_custom_button("prev", "panel_controls/button_prev.png");
  add_custom_button("next", "panel_controls/button_next.png");
  add_custom_button("stop", "panel_controls/button_stop.png");
  add_custom_button("repeat", "panel_controls/button_repeat.png");
  add_custom_button("shuffle", "panel_controls/button_shuffle.png");
  add_custom_button("expand_player", "panel_controls/button_expand_player.png");
  add_custom_slider("seekbar", "panel_controls/seekbar_track.png", "panel_controls/seekbar_thumb.png");
  atlas_add_texture("play", {"panel_controls/play.png"});
  atlas_add_texture("pause", {"panel_controls/pause.png"});
  atlas_add_texture("stop", {"panel_controls/stop.png"});
  atlas_add_texture("next", {"panel_controls/next.png"});
  atlas_add_texture("prev", {"panel_controls/prev.png"});
  atlas_add_texture_row(std::array{"repeat_off"s, "repeat"s, "repeat_album"s, "repeat_track"s},
                        std::array{"panel_controls/repeat.png"s});
  atlas_add_texture_row(std::array{"shuffle_off"s, "shuffle"s}, std::array{"panel_controls/shuffle.png"s});
  atlas_add_texture("expand_player", {"panel_controls/expand_player.png"});
  add_custom_button("panel_controls_love", "panel_controls/love.png");
  add_custom_button("panel_controls_unlove", "panel_controls/unlove.png");

  // panel_tracklist
  add_custom_button("inline_play", "panel_tracklist/inline_play.png");
  add_custom_button("inline_play_next", "panel_tracklist/inline_play_next.png");
  add_custom_button("inline_sort", "panel_tracklist/inline_sort.png");
  add_custom_button("inline_more", "panel_tracklist/inline_more.png");
  atlas_add_texture("love", {"panel_tracklist/love.png"});
  atlas_add_texture("sort_by", {"panel_tracklist/sort_by.png"});

  // panel_playlists
  add_custom_panel("panel_playlists_searchbar", "panel_playlists/searchbar.png");
  add_custom_panel("panel_playlists_header", "panel_playlists/header.png");
  atlas_add_texture("playlist_hovered", {"panel_playlists/playlist_hovered.png"});
  atlas_add_texture_row(
    std::array{"clear_search_idle"s, "clear_search_hovered"s, "clear_search_pressed"s, "clear_search_disabled"s},
    std::array{"panel_playlists/clear_search.png"s});
  atlas_add_texture("playlist_playing", {"panel_playlists/playlist_playing.png"});
  atlas_add_texture("button_add_playlist", {"panel_playlists/button_add_playlist.png"});

  // panel_top
  add_custom_button("add_tab", "panel_top/add_tab.png");
  atlas_add_texture("add_tab_icon", {"panel_top/add_tab_icon.png"});
  atlas_add_texture("left", {"panel_top/left.png"});
  atlas_add_texture("right", {"panel_top/right.png"});
  add_custom_button("button_decor_minimize", "panel_top/button_minimize.png");
  add_custom_button("button_decor_maximize", "panel_top/button_maximize.png");
  add_custom_button("button_decor_close", "panel_top/button_close.png");
  atlas_add_texture("icon_decor_minimize", {"panel_top/minimize.png"});
  atlas_add_texture("icon_decor_maximize", {"panel_top/maximize.png"});
  atlas_add_texture("icon_decor_close", {"panel_top/close.png"});
  atlas_add_texture("hamburger", {"panel_top/hamburger.png"});

  // actions
  atlas_add_texture("mini_player", {"actions/mini_player.png"});
  atlas_add_texture("search", {"actions/search.png"});
  atlas_add_texture("about", {"actions/about.png"});
  atlas_add_texture("add_to_playlist", {"actions/add_to_playlist.png"});
  atlas_add_texture("quit", {"actions/quit.png"});
  atlas_add_texture("love_track", {"actions/love_track.png"});
  atlas_add_texture("play_next", {"actions/play_next.png"});
  atlas_add_texture("delete_collection", {"actions/delete_collection.png"});
  atlas_add_texture("remove_from_queue", {"actions/remove_from_queue.png"});
  atlas_add_texture("rename_collection", {"actions/rename_collection.png"});
  atlas_add_texture("rescan", {"actions/rescan.png"});
  atlas_add_texture("set_sources", {"actions/set_sources.png"});
  atlas_add_texture("show_playlist_directory", {"actions/show_playlist_directory.png"});
  atlas_add_texture("show_in_album", {"actions/show_in_album.png"});
  atlas_add_texture("show_in_playlist", {"actions/show_in_playlist.png"});
  atlas_add_texture("unlove_track", {"actions/unlove_track.png"});
  atlas_add_texture("clear_queue", {"actions/clear_queue.png"});
  atlas_add_texture("save_queue_as_playlist", {"actions/save_queue_as_playlist.png"});
  atlas_add_texture("remove_from_playlist", {"actions/remove_from_playlist.png"});
  atlas_add_texture("play_track", {"actions/play_track.png"});
  atlas_add_texture("append_to_queue", {"actions/append_to_queue.png"});
  atlas_add_texture("name_asc", {"actions/name_asc.png"});
  atlas_add_texture("name_desc", {"actions/name_desc.png"});
  atlas_add_texture("artist_asc", {"actions/artist_asc.png"});
  atlas_add_texture("artist_desc", {"actions/artist_desc.png"});
  atlas_add_texture("title_asc", {"actions/title_asc.png"});
  atlas_add_texture("title_desc", {"actions/title_desc.png"});
  atlas_add_texture("settings", {"actions/settings.png"});
  atlas_add_texture("rename_playlist", {"actions/rename_playlist.png"});
  atlas_add_texture("delete_playlist", {"actions/delete_playlist.png"});
  atlas_add_texture("save_playlist_as_json", {"actions/save_playlist_as_json.png"});
  atlas_add_texture("pick_playlist_cover", {"actions/pick_playlist_cover.png"});
  atlas_add_texture("reset_playlist_cover", {"actions/reset_playlist_cover.png"});
  atlas_add_texture("add_playlist", {"actions/add_playlist.png"});
  atlas_add_texture("add_playlist_from_json", {"actions/add_playlist_from_json.png"});
  atlas_add_texture("add_smart_playlist", {"actions/add_smart_playlist.png"});
}
