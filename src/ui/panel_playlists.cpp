#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include "common/search_utils.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player.hpp"
#include "theme.hpp"
#include "ui/cover_art.hpp"
#include "ui/panel_playlists.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

SpriteAlbumCover::SpriteAlbumCover(Root& ui_, std::optional<db::playlist_id_t> playlist_id, vec2i cover_size_,
                                   bool is_add_button)
  : Sprite(ui_) {
  set_nine_slice_margin(1.0f);
  set_atlas_index(2);
  set_size(cover_size_);

  if (is_add_button) {
    set_texture("button_add_playlist", false);
  } else if (!set_cover(playlist_id)) {
    set_texture("cover_unknown", false);
  }
}

bool SpriteAlbumCover::set_cover(std::optional<db::playlist_id_t> playlist_id) {
  if (!playlist_id) { return false; }
  const auto& playlist = db::playlist_by_id(*playlist_id);
  if (!playlist.has_value()) { return false; }
  if (playlist->get().track_ids.empty()) { return false; }
  auto track_id = playlist->get().track_ids.front();
  std::optional<std::string> id;
  id = zincbox::ui::cover_art::get_texture_id(track_id, width > 64);
  if (!id) { return false; }
  set_texture(*id, false);
  return true;
}

zincgui::TextureAtlas& SpriteAlbumCover::get_texture_atlas() { return zincbox::ui::cover_art::texture_atlas(); }

void WidgetAlbumCover::update_highlight_status_from_player() {
  bool playing = player::get_playing_track() && playlist_id == player::get_playing_track()->playlist_id;
  auto& config = theme::config().panel_playlists;

  if (playing) {
    sprite_playing->set_is_drawn(true);
    label_title->set_text_color(config.title_color * 1.5f);
    label_author->set_text_color(config.author_color * 1.5f);
  } else {
    sprite_playing->set_is_drawn(false);
    label_title->set_text_color(config.title_color);
    label_author->set_text_color(config.author_color);
  }
}

WidgetAlbumCover::WidgetAlbumCover(Root& ui_, std::optional<size_t> playlist_id_, bool is_add_button_)
  : Button(ui_), playlist_id(playlist_id_) {
  m_is_add_button = is_add_button_;

  auto& settings = zincbox::settings();
  auto& config = theme::config().panel_playlists;
  float scale = zincbox::ui_scale();

  i32 cover_w = settings.interface.cover_size;

  i32 text_space = 0;
  if (config.show_title || config.show_author) { text_space += 8 * scale; }
  if (config.show_title) { text_space += 16 * scale; }
  if (config.show_author) { text_space += 16 * scale; }

  cover_size = vec2i{cover_w, cover_w};
  total_size = vec2i{cover_w, cover_w + text_space};

  set_clip_children(true);
  set_size(total_size.x, total_size.y);

  auto& sprite_cover = add_child<SpriteAlbumCover>(playlist_id_, cover_size, m_is_add_button);
  sprite_cover.set_parent_anchor(Anchor::TOP);
  sprite_cover.set_anchor(Anchor::TOP);

  label_title = &add_child<Label>();
  label_title->set_clip(false);
  label_author = &add_child<Label>();
  label_author->set_clip(false);

  if (playlist_id.has_value()) {
    auto& playlist = db::playlist_by_id(playlist_id.value())->get();
    label_title->set_text(playlist.name);
    label_author->set_text(playlist.author_pretty());
  } else if (m_is_add_button) {
    label_title->set_text(tr::get("playlist.add_new_placeholder"));
  }

  label_title->set_resize_to_text_extents(false);
  label_title->set_width(total_size.x);
  label_title->set_label_anchor(Anchor::TOP_LEFT);
  label_title->set_anchor(Anchor::TOP_LEFT);
  label_title->set_parent_anchor(Anchor::TOP_LEFT);

  label_author->set_resize_to_text_extents(false);
  label_author->set_width(total_size.x);
  label_author->set_label_anchor(Anchor::TOP_LEFT);
  label_author->set_anchor(Anchor::TOP_LEFT);
  label_author->set_parent_anchor(Anchor::TOP_LEFT);

  label_title->set_is_drawn(config.show_title);
  label_author->set_is_drawn(config.show_author);

  i32 current_y = cover_size.y + 8 * scale;
  if (config.show_title) {
    label_title->set_y(current_y);
    current_y += 16 * scale;
  }
  if (config.show_author) { label_author->set_y(current_y); }

  hover = &sprite_cover.add_child<Sprite>("playlist_hovered");
  hover->set_parent_anchor(Anchor::TOP);
  hover->set_anchor(Anchor::TOP);
  hover->set_nine_slice_margin(8.0f);
  hover->set_ignore_parents_layout(true);
  hover->set_size(cover_size.x, cover_size.y);
  hover->set_is_drawn(false);

  sprite_playing = &sprite_cover.add_child<Sprite>("playlist_playing");
  sprite_playing->set_parent_anchor(Anchor::TOP);
  sprite_playing->set_anchor(Anchor::TOP);
  sprite_playing->set_nine_slice_margin(8.0f);
  sprite_playing->set_ignore_parents_layout(true);
  sprite_playing->set_size(cover_size.x, cover_size.y);
  sprite_playing->set_is_drawn(false);

  slot_on_track_changed =
    player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
  update_highlight_status_from_player();
}

WidgetAlbumCover::~WidgetAlbumCover() { player::signal_on_track_changed.disconnect(slot_on_track_changed); }

void WidgetAlbumCover::draw() {
  // Button::draw();
}

void WidgetAlbumCover::update() {
  if (!is_mouse_hovering()) { is_hovered = false; }

  hover->set_is_drawn(is_hovered);

  auto& config = theme::config().panel_playlists;

  if (is_hovered && config.show_title && label_title->get_text_extents().x > total_size.x + 8) {
    label_title->set_x(label_title->get_x() - 1);
    if (label_title->get_x() <= -label_title->get_text_extents().x - 8) { label_title->set_x(total_size.x + 8); }
  } else {
    label_title->set_x(0);
  }

  Button::update();
}

void WidgetAlbumCover::event(Input::InputEventMouseMove& ev) {
  is_hovered = is_mouse_hovering();
  Button::event(ev);
}

PanelPlaylists::PanelPlaylists(Root& ui_) : ColorRect(ui_) {
  i32 PANEL_SEARCH_HEIGHT = 2 * PANEL_SEARCH_PADDING + 8 * zincbox::ui_scale() + zincbox::font_size();

  auto& config = theme::config().panel_playlists;
  set_color(config.color);
  set_clip_children(true);

  scrollbar = &add_child<ZincboxScrollbar>();
  scrollbar->set_anchor(Anchor::RIGHT);
  scrollbar->set_parent_anchor(Anchor::RIGHT);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });

  albums_container = &add_child<Widget>();
  albums_container->set_pos(0, PANEL_SEARCH_HEIGHT + 2 * PANEL_SEARCH_PADDING);

  panel_search = &add_child<Sprite>("panel_playlists_searchbar");
  panel_search->set_pos(PANEL_SEARCH_PADDING, PANEL_SEARCH_PADDING);
  panel_search->set_height(PANEL_SEARCH_HEIGHT);
  panel_search->set_layout("m:6 s:6 ltr fill expand");
  panel_search->set_nine_slice_margin(8.0f);
  panel_search->set_ignore_parents_layout(true);

  search_bar = &panel_search->add_child<TextInput>();
  search_bar->set_fixed_height(8 * zincbox::ui_scale() + zincbox::font_size());
  search_bar->set_on_text_changed([this]() { recreate(); });
  button_clear_search = &search_bar->add_child<Button>();

  button_sort_by = &panel_search->add_child<Button>();
  button_sort_by->set_max_width(24);
  button_sort_by->set_max_height(24);
  button_sort_by->on_press([this]() {
    if (this->on_button_sort_by_pressed) { this->on_button_sort_by_pressed(this->button_sort_by); }
  });
  button_sort_by->add_image("sort_by");

  button_clear_search->set_x(-2);
  button_clear_search->set_ignore_parents_layout(true);
  button_clear_search->set_texture("clear_search_idle", true);
  button_clear_search->set_texture_idle("clear_search_idle");
  button_clear_search->set_texture_hovered("clear_search_hovered");
  button_clear_search->set_texture_pressed("clear_search_pressed");
  button_clear_search->set_nine_slice_margin(0);
  button_clear_search->set_anchor(Anchor::RIGHT);
  button_clear_search->set_parent_anchor(Anchor::RIGHT);

  button_clear_search->on_press([this]() { this->search_bar->clear(); });
}

void PanelPlaylists::clear() {
  for (auto& w : album_widgets) {
    w->set_marked_for_deletion(true);
  }
  album_widgets.clear();
}

void PanelPlaylists::recreate() {
  clear();

  panel_search->set_is_updated(props.panel_search_visible);
  panel_search->set_is_drawn(props.panel_search_visible);

  scrollbar->set_is_updated(props.is_scrollable);
  scrollbar->set_is_drawn(props.is_scrollable && (scrollbar->get_content_size() > scrollbar->get_page_size()));

  button_sort_by->set_is_updated(props.button_sort_by_visible);
  button_sort_by->set_is_drawn(props.button_sort_by_visible);

  std::vector<size_t> playlist_ids_sorted;
  if (props.collection_id.has_value()) {
    auto c = db::collection_by_id(*props.collection_id);
    playlist_ids_sorted = c->get().playlist_ids();
  } else if (!props.playlist_ids.empty()) {
    playlist_ids_sorted = props.playlist_ids;
  } else {
    return;
  }

  if (props.sort_by == SortBy::NAME_AZ) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.name, lhs.author) < std::tie(rhs.name, rhs.author);
    });
  } else if (props.sort_by == SortBy::NAME_ZA) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.name, lhs.author) > std::tie(rhs.name, rhs.author);
    });
  } else if (props.sort_by == SortBy::AUTHOR_AZ) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.author, lhs.name) < std::tie(rhs.author, rhs.name);
    });
  } else if (props.sort_by == SortBy::AUTHOR_ZA) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.author, lhs.name) > std::tie(rhs.author, rhs.name);
    });
  }

  std::vector<size_t> playlist_ids_sorted_filtered =
    db::search_playlists(search_bar->label.get_text(), playlist_ids_sorted, 512);

  for (size_t playlist_id : playlist_ids_sorted_filtered) {
    auto* album_widget = &albums_container->add_child<WidgetAlbumCover>(playlist_id);
    album_widgets.emplace_back(album_widget);

    album_widget->on_press([this, playlist_id, album_widget]() {
      if (on_playlist_lmb) { on_playlist_lmb(playlist_id, album_widget); }
    });
    album_widget->on_press_rmb([this, playlist_id, album_widget]() {
      if (on_playlist_rmb) { on_playlist_rmb(playlist_id, album_widget); }
    });
  }

  if (props.collection_id.has_value() && props.collection_id == 0 && props.button_add_playlist_visible) {
    auto* album_widget = &albums_container->add_child<WidgetAlbumCover>(std::nullopt, true);
    album_widgets.emplace_back(album_widget);

    album_widget->on_press([this, album_widget]() {
      if (on_add_playlist_button_pressed) { on_add_playlist_button_pressed(album_widget); }
    });
  }

  reflow();
}

void PanelPlaylists::scroll_to_playlist(size_t playlist_id, bool immediate) {
  for (auto& album_widget : album_widgets) {
    if (album_widget->playlist_id == playlist_id) {
      target_scroll_px = album_widget->get_y();
      target_scroll_px = std::clamp(target_scroll_px, 0.0, std::max(0.0, (double)(content_height - height)));
      scrollbar->set_scroll_offset(target_scroll_px);
      if (immediate) {
        scroll_px = target_scroll_px;
        scrollbar->skip_anim();
      }
      break;
    }
  }
}

void PanelPlaylists::reflow() {
  auto& config = theme::config().panel_playlists;
  auto& settings = zincbox::settings();
  const float scale = zincbox::ui_scale();

  i32 cover_w = settings.interface.cover_size;
  i32 text_space = 0;
  if (config.show_title || config.show_author) { text_space += 8 * scale; }
  if (config.show_title) { text_space += 16 * scale; }
  if (config.show_author) { text_space += 16 * scale; }

  i32 item_w = cover_w;
  i32 item_h = cover_w + text_space;

  i32 min_spacing_x = std::max<i32>(0, config.horizontal_spacing * scale);
  i32 spacing_y = config.vertical_spacing * scale;

  i32 albums_area_width = albums_container->get_width();
  if (albums_area_width <= 0) { return; }

  i32 album_covers_in_one_row = (albums_area_width - min_spacing_x) / (item_w + min_spacing_x);
  if (album_covers_in_one_row <= 0) { album_covers_in_one_row = 1; }

  i32 total_items_w = album_covers_in_one_row * item_w;
  i32 total_gap_space = std::max<i32>(0, albums_area_width - total_items_w);
  i32 total_gaps_count = album_covers_in_one_row + 1;

  std::optional<std::string_view> prev_artist;
  std::optional<char32_t> prev_name_first_letter;
  i32 column = 0;
  i32 cover_y = spacing_y;

  for (auto& album_widget : album_widgets) {
    bool group_by_author = props.group && (props.sort_by == SortBy::AUTHOR_AZ || props.sort_by == SortBy::AUTHOR_ZA);
    bool group_by_name = props.group && (props.sort_by == SortBy::NAME_AZ || props.sort_by == SortBy::NAME_ZA);
    bool force_new_row = false;

    if (group_by_author && prev_artist && album_widget->label_author->get_text() != *prev_artist) {
      force_new_row = true;
    }

    std::optional<char32_t> name_first_letter;
    if (!album_widget->label_title->get_text().empty()) {
      name_first_letter = transform_to_base_char(album_widget->label_title->get_text()[0]);
      if (group_by_name && prev_name_first_letter && *name_first_letter != *prev_name_first_letter) {
        force_new_row = true;
      }
    }

    if (column >= album_covers_in_one_row || (force_new_row && column > 0)) {
      column = 0;
      cover_y += item_h + spacing_y;
    }

    i32 gap_before = (total_gap_space * (column + 1)) / total_gaps_count;
    i32 x_ = gap_before + column * item_w;
    album_widget->set_pos(x_, cover_y);

    column += 1;
    prev_artist = album_widget->label_author->get_text();
    prev_name_first_letter = name_first_letter;
  }

  i32 covers_total_bounds = album_widgets.empty() ? 0 : (cover_y + item_h + spacing_y);
  content_height = covers_total_bounds + (props.panel_search_visible ? (8 + panel_search->get_height()) : 0);

  scrollbar->set_content_size(content_height);
  scrollbar->set_page_size(height);
  scrollbar->set_height(height);
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(content_height - height)));
  albums_container->set_height(content_height);
}

void PanelPlaylists::input() { ColorRect::input(); }

void PanelPlaylists::update() {
  if (props_old != props) {
    recreate();
    props_old = props;
  }
  panel_search->set_width(width - (2 * PANEL_SEARCH_PADDING + scrollbar->get_width()));
  albums_container->set_width(props.is_scrollable ? width - scrollbar->get_width() : width);
  reflow();
  auto albums_container_prev_y = albums_container->get_y();
  albums_container->set_y(props.panel_search_visible
                            ? (panel_search->get_height() + 2 * PANEL_SEARCH_PADDING) - std::floor(scroll_px)
                            : -std::floor(scroll_px));

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  ColorRect::update();
}

void PanelPlaylists::show() {
  set_is_drawn(true);
  set_is_updated(true);
}

void PanelPlaylists::hide() {
  props.collection_id = std::nullopt;
  set_is_drawn(false);
  set_is_updated(false);
}

void PanelPlaylists::draw() { ColorRect::draw(); }

void PanelPlaylists::event(Input::InputEventMouseScroll& e) {
  if (props.is_scrollable && is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

float PanelPlaylists::get_scroll_px() const {
  if (!props.is_scrollable) { return 0.0f; }
  return target_scroll_px;
}

void PanelPlaylists::set_scroll_px(float px, bool immediate) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
  if (immediate) { scrollbar->skip_anim(); }
}

vec2i PanelPlaylists::get_content_size() const { return {width, content_height}; }
