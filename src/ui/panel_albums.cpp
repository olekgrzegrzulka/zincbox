#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "common/input.hpp"
#include "common/search_utils.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/player.hpp"
#include "panel_albums.hpp"
#include "theme.hpp"
#include "tr.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

SpriteAlbumCover::SpriteAlbumCover(UI& ui_, const std::string& id, vec2i cover_size_) : Sprite(ui_) {
  set_texture(id);
  set_size(cover_size_);
  set_nine_slice_margin(0.0f);
}

void WidgetAlbumCover::update_highlight_status_from_player() {
  bool playing = player::get_playing().has_value() && playlist_id == player::get_playing()->playlist_id;
  sprite_playing->set_is_drawn(playing);
  if (playing) {
    label_title->set_text_color(label_title_text_color * 1.5);
    label_author->set_text_color(label_author_text_color * 1.5);
  } else {
    label_title->set_text_color(label_title_text_color);
    label_author->set_text_color(label_author_text_color);
  }
}

WidgetAlbumCover::WidgetAlbumCover(UI& ui_, std::optional<size_t> playlist_id_, vec2i total_size_, vec2i cover_size_,
                                   bool is_add_button_)
  : Button(ui_), playlist_id(playlist_id_), total_size(total_size_), cover_size(cover_size_) {
  m_is_add_button = is_add_button_;
  label_title_text_color = theme::get_prop("playlist_title_text_color").as_rgba({255, 255, 255, 255});
  label_author_text_color = theme::get_prop("playlist_author_text_color").as_rgba({255, 255, 255, 255});
  set_clip_children(true);
  set_size(total_size.x, total_size.y);
  std::string sprite_cover_id;
  if (playlist_id.has_value()) {
    sprite_cover_id = std::to_string(playlist_id.value());
  } else if (m_is_add_button) {
    sprite_cover_id = "button_add_playlist";
  } else {
    sprite_cover_id = "cover_unknown";
  }
  auto& sprite_cover = add_child<SpriteAlbumCover>(sprite_cover_id, cover_size_);
  sprite_cover.set_parent_anchor(Anchor::TOP);
  sprite_cover.set_anchor(Anchor::TOP);
  label_title = &add_child<Label>();
  label_title->set_clip(false);
  label_author = &add_child<Label>();
  label_author->set_clip(false);
  if (playlist_id.has_value()) {
    auto& playlist = db::playlist_by_id(playlist_id.value())->get();
    label_title->set_text(playlist.name);
    label_author->set_text(playlist.author);
  } else if (m_is_add_button) {
    label_title->set_text(tr::get("playlist.add_new_placeholder"));
  }
  label_title->set_resize_to_text_extents(false);
  label_title->set_width(total_size.x);
  label_title->set_label_anchor(Anchor::TOP_LEFT);
  label_title->set_anchor(Anchor::TOP_LEFT);
  label_title->set_parent_anchor(Anchor::TOP_LEFT);
  label_title->set_text_color(theme::get_prop("playlist_title_text_color").as_rgba());
  label_title->set_y(cover_size.y + 8);

  label_author->set_resize_to_text_extents(false);
  label_author->set_width(total_size.x);
  label_author->set_label_anchor(Anchor::TOP_LEFT);
  label_author->set_anchor(Anchor::TOP_LEFT);
  label_author->set_parent_anchor(Anchor::TOP_LEFT);
  label_author->set_text_color(theme::get_prop("playlist_author_text_color").as_rgba());
  label_author->set_y(cover_size.y + 8 + 16);

  hover = &sprite_cover.add_child<Sprite>("playlist_hovered");
  hover->set_nine_slice_margin(8.0f);
  hover->set_ignore_parents_layout(true);
  hover->set_size(cover_size.x, cover_size.y);
  hover->set_is_drawn(false);

  sprite_playing = &sprite_cover.add_child<Sprite>("playlist_playing");
  sprite_playing->set_nine_slice_margin(0.0f);
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

  if (is_hovered && label_title->get_text_extents().x > total_size.x + 8) {
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

PanelAlbums::PanelAlbums(UI& ui_) : Sprite(ui_, "panel_albums") {
  set_clip_children(true);

  scrollbar = &add_child<ScrollBar>();
  scrollbar->set_anchor(Anchor::RIGHT);
  scrollbar->set_parent_anchor(Anchor::RIGHT);
  scrollbar->set_width(12);
  scrollbar->set_thumb_thickness(10);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);
  scrollbar->set_track_thickness(10);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) { target_scroll_px = scroll_offset; });

  albums_container = &add_child<Widget>();
  albums_container->set_pos(0, PANEL_SEARCH_HEIGHT + 2 * PANEL_SEARCH_PADDING);

  panel_search = &add_child<Sprite>("panel_albums_searchbar");
  panel_search->set_pos(PANEL_SEARCH_PADDING, PANEL_SEARCH_PADDING);
  panel_search->set_height(PANEL_SEARCH_HEIGHT);
  panel_search->set_layout("m:6 s:6 ltr fill expand");
  panel_search->set_nine_slice_margin(8.0f);
  panel_search->set_ignore_parents_layout(true);

  search_bar = &panel_search->add_child<TextInput>();
  search_bar->set_on_text_changed([this]() { recreate(); });
  button_clear_search = &search_bar->add_child<Button>();

  button_sort_by = &panel_search->add_child<Button>();
  button_sort_by->set_max_width(24);
  button_sort_by->set_max_height(24);
  button_sort_by->on_press([this]() {
    if (this->on_button_sort_by_pressed) { this->on_button_sort_by_pressed(this->button_sort_by); }
  });
  button_sort_by->add_image("sort_by");

  button_clear_search->set_width(20);
  button_clear_search->set_height(20);
  button_clear_search->set_x(-2);
  button_clear_search->set_ignore_parents_layout(true);
  button_clear_search->set_texture("clear_search_idle", false);
  button_clear_search->set_texture_idle("clear_search_idle");
  button_clear_search->set_texture_hovered("clear_search_hovered");
  button_clear_search->set_texture_pressed("clear_search_pressed");
  button_clear_search->set_anchor(Anchor::RIGHT);
  button_clear_search->set_parent_anchor(Anchor::RIGHT);

  button_clear_search->on_press([this]() { this->search_bar->clear(); });
}

void PanelAlbums::draw() { Sprite::draw(); }

void PanelAlbums::clear() {
  for (auto& w : album_widgets) {
    w->set_marked_for_deletion(true);
  }
  album_widgets.clear();
}

void PanelAlbums::recreate() {
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

  vec2i cover_widget_size = {props.cover_width + props.cover_min_horizontal_spacing,
                             props.cover_width + props.cover_min_vertical_spacing};
  vec2i cover_widget_cover_size = {props.cover_width, props.cover_width};

  std::vector<size_t> playlist_ids_sorted_filtered =
    db::search_playlists(search_bar->label.get_text(), playlist_ids_sorted, 512);

  for (size_t playlist_id : playlist_ids_sorted_filtered) {
    auto* album_widget =
      &albums_container->add_child<WidgetAlbumCover>(playlist_id, cover_widget_size, cover_widget_cover_size);
    album_widgets.emplace_back(album_widget);

    album_widget->on_press([this, playlist_id, album_widget]() {
      if (on_playlist_lmb) { on_playlist_lmb(playlist_id, album_widget); }
    });
    album_widget->on_press_rmb([this, playlist_id, album_widget]() {
      if (on_playlist_rmb) { on_playlist_rmb(playlist_id, album_widget); }
    });
  }

  if (props.collection_id.has_value() && props.collection_id == 0 && props.button_add_playlist_visible) {
    auto* album_widget =
      &albums_container->add_child<WidgetAlbumCover>(std::nullopt, cover_widget_size, cover_widget_cover_size, true);
    album_widgets.emplace_back(album_widget);

    album_widget->on_press([this, album_widget]() {
      if (on_add_playlist_button_pressed) { on_add_playlist_button_pressed(album_widget); }
    });
  }

  reflow();
}

void PanelAlbums::scroll_to_playlist(size_t playlist_id, bool immediate) {
  for (auto& album_widget : album_widgets) {
    if (album_widget->playlist_id == playlist_id) {
      target_scroll_px = album_widget->get_y();
      target_scroll_px = std::clamp(target_scroll_px, 0.0, std::max(0.0, (double)(content_height - height)));
      scrollbar->set_scroll_offset(target_scroll_px);
      if (immediate) { scroll_px = target_scroll_px; }
      break;
    }
  }
}

void PanelAlbums::reflow() {
  i32 cover_total_width = props.cover_width + props.cover_min_horizontal_spacing;
  i32 cover_total_height = props.cover_width + props.cover_min_vertical_spacing;
  i32 albums_area_width = albums_container->get_width();
  i32 album_covers_in_one_row = albums_area_width / cover_total_width;
  i32 space_left = albums_area_width - album_covers_in_one_row * cover_total_width;
  if (album_covers_in_one_row <= 0) { return; }
  std::optional<std::u32string_view> prev_artist;
  std::optional<char32_t> prev_name_first_letter;
  i32 column = 0;
  i32 cover_y = 0;
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
      cover_y += cover_total_height;
    }
    i32 x_ = space_left * (column + 1) / (album_covers_in_one_row + 1) + cover_total_width * column;
    album_widget->set_pos(x_, cover_y);
    column += 1;
    prev_artist = album_widget->label_author->get_text();
    prev_name_first_letter = name_first_letter;
  }
  i32 covers_total_bounds = album_widgets.empty() ? 0 : (cover_y + cover_total_height);
  content_height = covers_total_bounds + (props.panel_search_visible ? (8 + panel_search->get_height()) : 0);
  scrollbar->set_content_size(content_height);
  scrollbar->set_page_size(height);
  scrollbar->set_height(height);
  scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(content_height - height)));
  albums_container->set_height(content_height);
}

void PanelAlbums::update() {
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
  if (albums_container_prev_y != albums_container->get_y()) { ui.mark_dirty_recursive(albums_container); }

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  Sprite::update();
}

void PanelAlbums::event(Input::InputEventMouseScroll& e) {
  if (props.is_scrollable && is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

float PanelAlbums::get_scroll_px() const {
  if (!props.is_scrollable) { return 0.0f; }
  return target_scroll_px;
}

void PanelAlbums::set_scroll_px(float px) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
}

vec2i PanelAlbums::get_content_size() const { return {width, content_height}; }
