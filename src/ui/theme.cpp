#include <filesystem>
#include <initializer_list>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "common/color.hpp"
#include "common/debug.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "core/io.hpp"
#include "lib/inifile.h"
#include "lib/miniz/miniz.h"
#include "theme.hpp"
#include "tr.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"

namespace fs = std::filesystem;

static constexpr u8 resources_zip[] = {
#embed "resources.zip"
};

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
};

static std::unordered_map<std::string, theme::theme_prop, StringHash, std::equal_to<>> properties;
static std::unordered_set<std::string, StringHash, std::equal_to<>> props_not_found;
static std::unordered_map<std::string, std::vector<uint8_t>, StringHash, std::equal_to<>> resources;
static std::string resources_ttf_path;
static std::set<std::string> languages;

theme::theme_prop theme::get_prop(std::string_view prop) {
  if (auto it = properties.find(prop); it != properties.end()) { return it->second; }
  if (!props_not_found.contains(prop)) {
    std::string prop_str(prop);
    props_not_found.emplace(prop_str);
    out::debug_warn("theme has no property {}", prop_str);
  }
  return theme_prop{std::monostate()};
}

i32 theme::get_button_nine_slice_margin(std::string_view name) {
  return get_prop(std::string(name) + "_button_nine_slice_margin")
    .as_i32(get_prop("button_nine_slice_margin").as_i32());
}

void load_resources() {
  if (!resources.empty()) { return; }

  mz_zip_archive zip_archive{};
  resources.clear();
  resources_ttf_path.clear();

  if (!mz_zip_reader_init_mem(&zip_archive, resources_zip, sizeof(resources_zip), 0)) {
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
    if (filename.starts_with("lang/") && filename.ends_with(".json")) { languages.insert(file_stat.m_filename); }

    resources[std::move(filename)] = std::move(buffer);
  }
  mz_zip_reader_end(&zip_archive);
}

std::set<std::string> theme::get_themes() {
  std::set<std::string> ret;

  for (const auto& entry : fs::directory_iterator(io::get_themes_path())) {
    if (fs::is_regular_file(entry.path() / "theme.cfg")) {
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
  ini::inifile ini;

  if (!load_theme_from_resources) {
    // Check if the theme exists in the themes directory
    if (!fs::is_directory(theme_path)) {
      out::warn("no theme found at {}", std::string{theme_path});
      load_theme("", ui);
      return;
    }

    // Try to load theme.cfg, else fallback to default theme
    bool success = ini.load(io::get_themes_path() / theme_name / "theme.cfg");
    bool invalid_theme = !success || !ini.contains("theme");
    if (invalid_theme) {
      out::warn("failed to load theme {} invalid or missing theme.cfg", theme_name);
      load_theme("", ui);
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
      ui.set_font_face(font_path, 14);
    } else {
      out::warn("no ttf file found in {}", std::string{theme_name});
      load_theme("", ui);
      return;
    }
  } else {
    load_resources();
    if (!resources.contains("theme.cfg")) {
      out::critical("theme.cfg not found");
      exit(1);
    }
    std::string str_theme_cfg(reinterpret_cast<const char*>(resources["theme.cfg"].data()),
                              resources["theme.cfg"].size());
    ini.from_string(str_theme_cfg);
    if (!ini.contains("theme")) {
      out::critical("failed to parse theme.cfg");
      exit(1);
    }

    if (resources_ttf_path.empty()) {
      out::critical("no ttf file found in default theme");
      exit(1);
    }
    ui.set_font_face_from_data(resources[resources_ttf_path].data(), resources[resources_ttf_path].size(), 14);
  }

  // Parse theme.cfg
  properties.clear();
  for (const auto& [key, str_value2] : ini["theme"]) {
    theme::theme_prop prop;
    std::string str_value = str_value2.as<std::string>();
    const char* first = str_value.data();
    const char* last = first + str_value.size();

    if (auto color = color_utils::parse_color(str_value); color.has_value()) {
      prop.value = color.value();
    } else {
      i32 i;
      auto [ptr_i, ec_i] = std::from_chars(first, last, i);
      if (ec_i == std::errc{} && ptr_i == last) {
        prop.value = i;
      } else {
        double d;
        auto [ptr_d, ec_d] = std::from_chars(first, last, d);
        if (ec_d == std::errc{} && ptr_d == last) {
          prop.value = d;
        } else {
          prop.value = str_value;
        }
      }
    }
    properties[key] = std::move(prop);
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

  auto add_custom_button = [&atlas_add_texture, &atlas_add_texture_row](const std::string& name) {
    std::array<std::string, 4> ids = {name + "_idle", name + "_hovered", name + "_pressed", name + "_disabled"};
    if (!atlas_add_texture_row(ids, {name, "button"})) {
      atlas_add_texture(name + "_disabled", {name + "_disabled", name, "button_disabled"});
      atlas_add_texture(name + "_hovered", {name + "_hovered", name, "button_hovered"});
      atlas_add_texture(name + "_idle", {name + "_idle", name, "button_idle"});
      atlas_add_texture(name + "_pressed", {name + "_pressed", name, "button_pressed"});
    }
  };

  auto add_custom_slider = [&atlas_add_texture_row](const std::string& name) {
    std::array<std::string, 3> thumb_ids = {name + "_thumb_idle", name + "_thumb_hovered", name + "_thumb_pressed"};
    std::array<std::string, 2> track_ids = {name + "_track_inactive", name + "_track_active"};
    atlas_add_texture_row(thumb_ids, {name + "_thumb", "slider_thumb"});
    atlas_add_texture_row(track_ids, {name + "_track", "slider_track"});
  };

  auto add_custom_panel = [&atlas_add_texture](const std::string& name) {
    atlas_add_texture("panel_" + name, {"panel_" + name, "panel"});
  };

  // ui
  atlas_add_texture_row({"button_idle", "button_hovered", "button_pressed", "button_disabled"}, {"button"});
  atlas_add_texture("combo_box_button_contract");
  atlas_add_texture("combo_box_button_expand");
  atlas_add_texture("combo_box");
  atlas_add_texture("combo_box_focused");
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
  add_custom_button("inline_play");
  add_custom_button("inline_play_next");
  add_custom_button("inline_sort");
  add_custom_button("inline_more");
  atlas_add_texture("panel");
  add_custom_panel("albums");
  add_custom_panel("albums_searchbar");
  add_custom_panel("controls");
  add_custom_panel("playlist_header");
  add_custom_panel("popup");
  add_custom_panel("tabbar");
  add_custom_panel("top");
  add_custom_panel("tracks");
  add_custom_panel("combo");
  add_custom_button("button_combo");
  add_custom_slider("seekbar");
  atlas_add_texture("track_bg1");
  atlas_add_texture("track_bg2");
  atlas_add_texture("track_bg_playing");
  atlas_add_texture("track_bg_selected1");
  atlas_add_texture("track_bg_selected2");
  atlas_add_texture("track_hovered");
  atlas_add_texture("playlist_hovered");
  atlas_add_texture_row({"tab_active_idle", "tab_active_hovered", "tab_active_pressed", "tab_active_disabled"},
                        {"tab_active"});
  atlas_add_texture_row({"tab_inactive_idle", "tab_inactive_hovered", "tab_inactive_pressed", "tab_inactive_disabled"},
                        {"tab_inactive"});
  atlas_add_texture("popover_panel");
  atlas_add_texture("popover_arrow");
  atlas_add_texture("popover_arrow_inverted");
  add_custom_button("add_tab");
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
  atlas_add_texture_row({"repeat_off", "repeat", "repeat_album", "repeat_track"}, {"icons/repeat"});
  atlas_add_texture_row({"shuffle_off", "shuffle"}, {"icons/shuffle"});
  atlas_add_texture("settings", {"icons/settings"});
  atlas_add_texture("search", {"icons/search"});
  atlas_add_texture_row({"clear_search_idle", "clear_search_hovered", "clear_search_pressed", "clear_search_disabled"},
                        {"icons/clear_search"});
  atlas_add_texture("sort_by", {"icons/sort_by"});
  atlas_add_texture("button_add_playlist", {"icons/button_add_playlist"});
  atlas_add_texture("cover_unknown");
  atlas_add_texture("playlist_playing");
  atlas_add_texture("insert_cursor");
  atlas.set_fallback_texture("cover_unknown");
}
