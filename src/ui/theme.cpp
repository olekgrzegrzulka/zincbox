#include <filesystem>
#include <initializer_list>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cmrc/cmrc.hpp>
#include <glaze/glaze.hpp>
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/serialized_state.hpp" // for glz::meta<rgba> specialization
#include "common/types.hpp"
#include "core/io.hpp"
#include "core/settings.hpp"
#include "core/zincbox.hpp"
#include "lib/miniz/miniz.h"
#include "theme.hpp"
#include "theme_config.hpp"
#include "tr.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"

namespace fs = std::filesystem;

CMRC_DECLARE(zincbox_resources);

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

static std::unordered_map<std::string, std::vector<uint8_t>, StringHash, std::equal_to<>> resources;
static std::string resources_ttf_path;
static std::set<std::string> languages;
static ThemeConfig config_;

static constexpr std::vector<uint8_t> NO_RESOURCE = {};

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

  mz_zip_archive zip_archive{};
  resources.clear();
  resources_ttf_path.clear();

  auto cmrc_fs = cmrc::zincbox_resources::get_filesystem();

  if (!cmrc_fs.exists("theme.zip")) {
    out::critical("theme.zip not found in cmrc");
    exit(1);
  }

  auto file = cmrc_fs.open("theme.zip");
  out::warn("s!");

  if (!mz_zip_reader_init_mem(&zip_archive, file.begin(), file.size(), 0)) {
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
    if (filename.starts_with("lang/") && filename.ends_with(".json")) {
      fs::path file_path{file_stat.m_filename};
      languages.insert(file_path.stem());
    }

    resources[std::move(filename)] = std::move(buffer);
  }
  mz_zip_reader_end(&zip_archive);
}

std::set<std::string> theme::get_themes() {
  std::set<std::string> ret;

  for (const auto& entry : fs::directory_iterator(io::get_themes_path())) {
    if (fs::is_regular_file(entry.path() / "theme.json")) {
      const std::string name = entry.path().filename().string();
      ret.insert(name);
    }
  }

  return ret;
}

std::set<std::string> theme::get_languages() {
  load_resources();
  return languages;
}

static bool load_language_from_resource(std::string_view language) {
  load_resources();
  std::string resource_name = std::string("lang/") + std::string(language) + ".json";
  auto it = resources.find(resource_name);
  if (it == resources.end()) { return false; }

  std::string content(reinterpret_cast<const char*>(it->second.data()), it->second.size());
  return tr::load_from_string(content);
}

static bool load_language_from_theme_path(const fs::path& theme_path, std::string_view language) {
  fs::path file_path = theme_path / "lang" / (std::string(language) + ".json");
  if (!fs::is_regular_file(file_path)) { return false; }
  return tr::load_from_file(file_path.string());
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

void theme::load_default_theme(UI& ui, std::string_view language) { load_theme("", ui, language); }

void theme::load_theme(std::string_view theme_name, UI& ui, std::string_view language) {
  const bool load_theme_from_resources = theme_name == "";
  if (load_theme_from_resources) { load_resources(); }

  fs::path theme_path(io::get_themes_path() / theme_name);

  if (!load_theme_from_resources) {
    // Check if the theme exists in the themes directory
    if (!fs::is_directory(theme_path)) {
      out::warn("no theme found at {}", std::string{theme_path});
      load_theme("", ui, language);
      return;
    }

    std::string theme_json_path = io::get_themes_path() / theme_name / "theme.json";
    auto error = glz::read_file_json(config_, theme_json_path, std::string{});
    if (error) {
      out::warn("failed to load theme {} invalid or missing theme.json", theme_name);
      load_theme("", ui, language);
      return;
    }

    // Try to load a font file, else fallback to default theme
    std::string font_path = "";
    for (auto const& dir_entry : fs::recursive_directory_iterator(theme_path)) {
      if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".ttf") {
        font_path = dir_entry.path();
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

  auto atlas_add_texture = [&load_theme_from_resources, &theme_path,
                            &atlas](const std::string& id, std::vector<std::string> filenames = {}) -> bool {
    if (filenames.size() == 0) { filenames = {id}; }
    if (!load_theme_from_resources) {
      for (const std::string& filename : filenames) {
        if (fs::is_regular_file((theme_path / (filename + ".png")))) {
          atlas.add_texture(id, (theme_path / (filename + ".png")).c_str());
          return true;
        } else if (fs::is_regular_file((theme_path / (filename + ".PNG")))) {
          atlas.add_texture(id, (theme_path / (filename + ".PNG")).c_str());
          return true;
        }
      }
    }

    out::debug_warn("theme has no {}.png, loading from default theme", filenames[0]);
    load_resources();
    for (const std::string& filename : filenames) {
      auto it = resources.find(filename + ".png");
      if (it == resources.end()) {
        it = resources.find(filename + ".PNG");
        if (it == resources.end()) { continue; }
      }
      i32 w, h, channels;
      u8* img = stbi_load_from_memory(it->second.data(), it->second.size(), &w, &h, &channels, STBI_rgb_alpha);
      if (!img) { continue; }
      atlas.add_texture(id, img, w, h);
      stbi_image_free(img);

      return true;
    }
    return false;
  };

  auto atlas_add_texture_row = [&load_theme_from_resources, &theme_path,
                                &atlas](std::span<const std::string> ids,
                                        std::span<const std::string> filenames = {}) -> bool {
    if (filenames.empty()) { return false; }

    if (!load_theme_from_resources) {
      for (const std::string& filename : filenames) {
        if (fs::is_regular_file((theme_path / (filename + ".png")))) {
          atlas.add_texture_row(ids, (theme_path / (filename + ".png")).string());
          return true;
        } else if (fs::is_regular_file((theme_path / (filename + ".PNG")))) {
          atlas.add_texture_row(ids, (theme_path / (filename + ".PNG")).string());
          return true;
        }
      }
    }

    out::debug_warn("theme has no {}.png, loading from default theme", filenames[0]);
    load_resources();
    for (const std::string& filename : filenames) {
      auto it = resources.find(filename + ".png");
      if (it == resources.end()) {
        it = resources.find(filename + ".PNG");
        if (it == resources.end()) { continue; }
      }

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

  auto add_custom_button = [&atlas_add_texture, &atlas_add_texture_row](const std::string& name) {
    std::array<std::string, 4> ids = {name + "_idle", name + "_hovered", name + "_pressed", name + "_disabled"};
    if (!atlas_add_texture_row(ids, std::array{name, "button"s})) {
      atlas_add_texture(name + "_disabled", {name + "_disabled", name, "button_disabled"});
      atlas_add_texture(name + "_hovered", {name + "_hovered", name, "button_hovered"});
      atlas_add_texture(name + "_idle", {name + "_idle", name, "button_idle"});
      atlas_add_texture(name + "_pressed", {name + "_pressed", name, "button_pressed"});
    }
  };

  auto add_custom_slider = [&atlas_add_texture_row](const std::string& name) {
    std::array<std::string, 3> thumb_ids = {name + "_thumb_idle", name + "_thumb_hovered", name + "_thumb_pressed"};
    std::array<std::string, 2> track_ids = {name + "_track_inactive", name + "_track_active"};
    atlas_add_texture_row(thumb_ids, std::array{name + "_thumb"s, "slider_thumb"s});
    atlas_add_texture_row(track_ids, std::array{name + "_track"s, "slider_track"s});
  };

  auto add_custom_panel = [&atlas_add_texture](const std::string& name) { atlas_add_texture(name, {name, "panel"}); };

  // ui
  atlas_add_texture_row(std::array{"button_idle"s, "button_hovered"s, "button_pressed"s, "button_disabled"s},
                        std::array{"button"s});
  add_custom_button("combobox");
  atlas_add_texture("combobox_contract");
  atlas_add_texture("combobox_expand");
  add_custom_panel("panel_combobox");
  add_custom_button("combobox_item");
  atlas_add_texture("dim");
  atlas_add_texture("red");
  add_custom_slider("slider");
  add_custom_slider("scrollbar");
  add_custom_slider("volume_bar");
  add_custom_button("spinner_decrease");
  add_custom_button("spinner_increase");
  atlas_add_texture("text_input_caret");
  atlas_add_texture("text_input_focused");
  atlas_add_texture("text_input_idle");
  atlas_add_texture("splitter");
  atlas_add_texture("tooltip");
  add_custom_button("checkbox");
  atlas_add_texture("checkbox_check");
  // player
  add_custom_button("play_pause");
  add_custom_button("prev");
  add_custom_button("next");
  add_custom_button("stop");
  add_custom_button("repeat");
  add_custom_button("shuffle");
  add_custom_button("expand_player");
  add_custom_button("inline_play");
  add_custom_button("inline_play_next");
  add_custom_button("inline_sort");
  add_custom_button("inline_more");
  atlas_add_texture("panel");
  atlas_add_texture("panel_dark");
  add_custom_panel("panel_albums_searchbar");
  add_custom_panel("panel_playlist_header");
  add_custom_panel("panel_popup");
  add_custom_slider("seekbar");
  atlas_add_texture("track_bg1");
  atlas_add_texture("track_bg2");
  atlas_add_texture("track_bg_playing");
  atlas_add_texture("track_bg_selected1");
  atlas_add_texture("track_bg_selected2");
  atlas_add_texture("track_hovered");
  atlas_add_texture("playlist_hovered");
  atlas_add_texture_row(
    std::array{"tab_active_idle"s, "tab_active_hovered"s, "tab_active_pressed"s, "tab_active_disabled"s},
    std::array{"tab_active"s});
  atlas_add_texture_row(
    std::array{"tab_inactive_idle"s, "tab_inactive_hovered"s, "tab_inactive_pressed"s, "tab_inactive_disabled"s},
    std::array{"tab_inactive"s});
  atlas_add_texture("popover_panel");
  atlas_add_texture("popover_arrow");
  atlas_add_texture("popover_arrow_inverted");
  add_custom_button("add_tab");
  atlas_add_texture("add_tab_icon");
  add_custom_button("button_popover");
  atlas_add_texture("notification");
  // icons
  atlas_add_texture("left", {"icons/left"});
  atlas_add_texture("right", {"icons/right"});
  atlas_add_texture("love", {"icons/love"});
  atlas_add_texture("play", {"icons/play"});
  atlas_add_texture("pause", {"icons/pause"});
  atlas_add_texture("stop", {"icons/stop"});
  atlas_add_texture("next", {"icons/next"});
  atlas_add_texture("prev", {"icons/prev"});
  atlas_add_texture("expand_player", {"icons/expand_player"});
  atlas_add_texture("mini_player", {"icons/mini_player"});
  atlas_add_texture_row(std::array{"repeat_off"s, "repeat"s, "repeat_album"s, "repeat_track"s},
                        std::array{"icons/repeat"s});
  atlas_add_texture_row(std::array{"shuffle_off"s, "shuffle"s}, std::array{"icons/shuffle"s});
  atlas_add_texture("hamburger", {"icons/hamburger"});
  atlas_add_texture("search", {"icons/actions/search"});
  atlas_add_texture_row(
    std::array{"clear_search_idle"s, "clear_search_hovered"s, "clear_search_pressed"s, "clear_search_disabled"s},
    std::array{"icons/clear_search"s});
  atlas_add_texture("sort_by", {"icons/sort_by"});
  atlas_add_texture("button_add_playlist", {"icons/actions/button_add_playlist"});
  atlas_add_texture("about", {"icons/actions/about"});
  atlas_add_texture("add_to_playlist", {"icons/actions/add_to_playlist"});
  atlas_add_texture("quit", {"icons/actions/quit"});
  atlas_add_texture("love_track", {"icons/actions/love_track"});
  atlas_add_texture("play_next", {"icons/actions/play_next"});
  atlas_add_texture("delete_collection", {"icons/actions/delete_collection"});
  atlas_add_texture("remove_from_queue", {"icons/actions/remove_from_queue"});
  atlas_add_texture("rename_collection", {"icons/actions/rename_collection"});
  atlas_add_texture("rescan", {"icons/actions/rescan"});
  atlas_add_texture("set_sources", {"icons/actions/set_sources"});
  atlas_add_texture("show_playlist_directory", {"icons/actions/show_playlist_directory"});
  atlas_add_texture("show_in_album", {"icons/actions/show_in_album"});
  atlas_add_texture("show_in_playlist", {"icons/actions/show_in_playlist"});
  atlas_add_texture("unlove_track", {"icons/actions/unlove_track"});
  atlas_add_texture("clear_queue", {"icons/actions/clear_queue"});
  atlas_add_texture("save_queue_as_playlist", {"icons/actions/save_queue_as_playlist"});
  atlas_add_texture("remove_from_playlist", {"icons/actions/remove_from_playlist"});
  atlas_add_texture("play_track", {"icons/actions/play_track"});
  atlas_add_texture("append_to_queue", {"icons/actions/append_to_queue"});
  atlas_add_texture("name_asc", {"icons/actions/name_asc"});
  atlas_add_texture("name_desc", {"icons/actions/name_desc"});
  atlas_add_texture("artist_asc", {"icons/actions/artist_asc"});
  atlas_add_texture("artist_desc", {"icons/actions/artist_desc"});
  atlas_add_texture("title_asc", {"icons/actions/title_asc"});
  atlas_add_texture("title_desc", {"icons/actions/title_desc"});
  atlas_add_texture("settings", {"icons/actions/settings"});
  atlas_add_texture("rename_playlist", {"icons/actions/rename_playlist"});
  atlas_add_texture("delete_playlist", {"icons/actions/delete_playlist"});
  atlas_add_texture("save_playlist_as_json", {"icons/actions/save_playlist_as_json"});
  atlas_add_texture("pick_playlist_cover", {"icons/actions/pick_playlist_cover"});
  atlas_add_texture("reset_playlist_cover", {"icons/actions/reset_playlist_cover"});
  atlas_add_texture("add_playlist", {"icons/actions/add_playlist"});
  atlas_add_texture("add_playlist_from_json", {"icons/actions/add_playlist_from_json"});
  atlas_add_texture("add_smart_playlist", {"icons/actions/add_smart_playlist"});
  add_custom_button("button_decor_minimize");
  add_custom_button("button_decor_maximize");
  add_custom_button("button_decor_close");
  atlas_add_texture("icon_decor_minimize", {"icons/icon_decor_minimize"});
  atlas_add_texture("icon_decor_maximize", {"icons/icon_decor_maximize"});
  atlas_add_texture("icon_decor_close", {"icons/icon_decor_close"});

  atlas_add_texture("cover_unknown");
  atlas_add_texture("playlist_playing");
  atlas_add_texture("insert_cursor");
  atlas.set_fallback_texture("cover_unknown");
}
