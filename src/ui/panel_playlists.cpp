#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "common/search_utils.hpp"
#include "core/musicdb/collection.hpp"
#include "core/musicdb/musicdb.hpp"
#include "core/musicdb/playlist.hpp"
#include "core/musicdb/types.hpp"
#include "core/player/player.hpp"
#include "theme.hpp"
#include "ui/cover_art.hpp"
#include "ui/panel_playlists.hpp"
#include "ui/theme_config.hpp"
#include "ui/tr.hpp"
#include "ui/zb_widgets.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/input.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/scrollbar.hpp"
#include "ui/zincgui/slider.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/text_input.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

SpriteAlbumCover::SpriteAlbumCover(Root& ui_, std::optional<db::playlist_id_t> playlist_id, vec2i cover_size_,
                                   bool is_add_button)
  : Sprite(ui_) {
  nine_slice_margin(1.0f).atlas_index(2).size(cover_size_);

  if (is_add_button) {
    texture("button_add_playlist", false);
  } else if (!cover(playlist_id)) {
    texture("cover_unknown", false);
  }
}

zincgui::TextureAtlas& SpriteAlbumCover::texture_atlas() { return zincbox::ui::cover_art::texture_atlas(); }

bool SpriteAlbumCover::cover(std::optional<db::playlist_id_t> playlist_id) {
  if (!playlist_id) { return false; }
  const auto& playlist = db::playlist_by_id(*playlist_id);
  if (!playlist.has_value()) { return false; }
  if (playlist->get().track_ids.empty()) { return false; }
  auto track_id = playlist->get().track_ids.front();
  std::optional<std::string> id;
  id = zincbox::ui::cover_art::get_texture_id(track_id, width() > 64);
  if (!id) { return false; }
  texture(*id, false);
  return true;
}

WidgetAlbumCover::WidgetAlbumCover(Root& ui_, std::optional<size_t> playlist_id_, bool is_add_button_)
  : Button(ui_), m_playlist_id(playlist_id_) {
  fixed_height(0);

  m_is_add_button = is_add_button_;

  auto& settings = zincbox::settings();
  auto& config = theme::config().panel_playlists;
  float scale = zincbox::ui_scale();

  i32 cover_w = settings.interface.cover_size;

  i32 text_space = 0;
  if (config.show_title || config.show_author) { text_space += 8 * scale; }
  if (config.show_title) { text_space += 16 * scale; }
  if (config.show_author) { text_space += 16 * scale; }

  m_cover_size = vec2i{cover_w, cover_w};
  m_total_size = vec2i{cover_w, cover_w + text_space};

  is_self_drawn(false).clip_children(true).size(m_total_size.x, m_total_size.y);

  auto& sprite_cover = add_child<SpriteAlbumCover>(playlist_id_, m_cover_size, m_is_add_button);
  sprite_cover.parent_anchor(Anchor::TOP).anchor(Anchor::TOP);

  m_label_title = &add_child<Label>();
  m_label_title->clip(false);
  m_label_author = &add_child<Label>();
  m_label_author->clip(false).font_size(zincbox::font_size_sm()).ellipsis(true);

  if (m_playlist_id.has_value()) {
    auto& playlist = db::playlist_by_id(m_playlist_id.value())->get();
    m_label_title->text(playlist.name);
    m_label_author->text(playlist.author_pretty());
  } else if (m_is_add_button) {
    m_label_title->text(tr::get("playlist.add_new_placeholder"));
  }

  m_label_title->resize_to_text_extents(false)
    .width(m_total_size.x)
    .label_anchor(Anchor::TOP_LEFT)
    .anchor(Anchor::TOP_LEFT)
    .parent_anchor(Anchor::TOP_LEFT)
    .is_drawn(config.show_title);

  m_label_author->resize_to_text_extents(false)
    .width(m_total_size.x)
    .label_anchor(Anchor::TOP_LEFT)
    .anchor(Anchor::TOP_LEFT)
    .parent_anchor(Anchor::TOP_LEFT)
    .is_drawn(config.show_author);

  i32 current_y = m_cover_size.y + (8 * scale);
  if (config.show_title) {
    m_label_title->y(current_y);
    current_y += 16 * scale;
  }
  if (config.show_author) { m_label_author->y(current_y); }

  m_hover = &sprite_cover.add_child<Sprite>("playlist_hovered");
  m_hover->parent_anchor(Anchor::TOP)
    .anchor(Anchor::TOP)
    .nine_slice_margin(8.0f)
    .ignore_parents_layout(true)
    .size(m_cover_size.x, m_cover_size.y)
    .is_drawn(false);

  m_sprite_playing = &sprite_cover.add_child<Sprite>("playlist_playing");
  m_sprite_playing->parent_anchor(Anchor::TOP)
    .anchor(Anchor::TOP)
    .nine_slice_margin(8.0f)
    .ignore_parents_layout(true)
    .size(m_cover_size.x, m_cover_size.y)
    .is_drawn(false);

  m_slot_on_track_changed =
    player::signal_on_track_changed.connect([this]() -> void { update_highlight_status_from_player(); });
  update_highlight_status_from_player();
}

WidgetAlbumCover::~WidgetAlbumCover() { player::signal_on_track_changed.disconnect(m_slot_on_track_changed); }

void WidgetAlbumCover::update() {
  if (is_mouse_hovering() && theme::config().panel_playlists.show_title &&
      m_label_title->text_extents().x > m_total_size.x + 8) {
    m_label_title->x(m_label_title->x() - 1);
    if (m_label_title->x() <= -m_label_title->text_extents().x - 8) { m_label_title->x(m_total_size.x + 8); }
  }

  Button::update();
}

void WidgetAlbumCover::on_hovered() {
  m_hover->is_drawn(true);
  Button::on_hovered();
}

void WidgetAlbumCover::on_unhovered() {
  m_hover->is_drawn(false);
  m_label_title->x(0);
  Button::on_unhovered();
}

void WidgetAlbumCover::update_highlight_status_from_player() {
  bool playing = player::get_playing_track() && m_playlist_id == player::get_playing_track()->playlist_id;
  auto& config = theme::config().panel_playlists;

  if (playing) {
    m_sprite_playing->is_drawn(true);
    m_label_title->text_color(config.title_color * 1.5f);
    m_label_author->text_color(config.author_color * 1.5f);
  } else {
    m_sprite_playing->is_drawn(false);
    m_label_title->text_color(config.title_color);
    m_label_author->text_color(config.author_color);
  }
}

PanelPlaylists::PanelPlaylists(Root& ui_) : ColorRect(ui_) {
  const i32 PANEL_SEARCH_HEIGHT = (2 * PANEL_SEARCH_PADDING) + zincgui::default_settings::text_input_size.y;

  auto& config = theme::config().panel_playlists;
  color(config.color).clip_children(true);

  m_scrollbar = &add_child<zincgui::ScrollBar>();
  m_scrollbar->anchor(Anchor::RIGHT)
    .parent_anchor(Anchor::RIGHT)
    .orientation(SliderOrientation::VERTICAL)
    .on_value_changed([this](i32 /* old */, i32 scroll_offset) { m_target_scroll_px = scroll_offset; });

  m_albums_container = &add_child<Widget>();
  m_albums_container->pos(0, PANEL_SEARCH_HEIGHT + (2 * PANEL_SEARCH_PADDING));

  m_panel_search = &add_child<Sprite>("panel_playlists_searchbar");
  m_panel_search->pos(PANEL_SEARCH_PADDING, PANEL_SEARCH_PADDING)
    .height(PANEL_SEARCH_HEIGHT)
    .layout("m:6 s:6 ltr fill expand")
    .nine_slice_margin(8.0f)
    .ignore_parents_layout(true);

  m_search_bar = &m_panel_search->add_child<TextInput>();
  m_search_bar->fixed_height(m_search_bar->height()).on_text_changed([this]() { recreate(); });

  m_button_sort_by = &m_panel_search->add_child<Button>();
  m_button_sort_by->max_width(24)
    .max_height(24)
    .on_press([this]() {
      if (m_on_button_sort_by_pressed) { m_on_button_sort_by_pressed(m_button_sort_by); }
    })
    .add_image("sort_by");

  m_button_clear_search = &m_search_bar->add_child<Button>();
  m_button_clear_search->x(-2)
    .ignore_parents_layout(true)
    .texture("clear_search_idle", true)
    .fixed_size(m_button_clear_search->size())
    .texture_idle("clear_search_idle")
    .texture_hovered("clear_search_hovered")
    .texture_pressed("clear_search_pressed")
    .nine_slice_margin(0)
    .anchor(Anchor::RIGHT)
    .parent_anchor(Anchor::RIGHT)
    .on_press([this]() -> void { m_search_bar->clear(); });
}

void PanelPlaylists::input() { ColorRect::input(); }

void PanelPlaylists::update() {
  if (m_props_old != m_props) {
    recreate();
  } else if (m_width_old != width() || m_height_old != height()) {
    m_width_old = width();
    m_height_old = height();
    reflow();
  }

  m_albums_container->y(m_props.panel_search_visible
                          ? (m_panel_search->height() + 2 * PANEL_SEARCH_PADDING) - std::floor(m_scroll_px)
                          : -std::floor(m_scroll_px));

  double t = std::clamp(std::abs(m_scroll_px - m_target_scroll_px) * 0.004, 0.4, 0.8);
  m_scroll_px = std::lerp(m_scroll_px, m_target_scroll_px, t);

  ColorRect::update();
}

void PanelPlaylists::event(Input::InputEventMouseScroll& e) {
  if (m_props.is_scrollable && is_mouse_hovering()) {
    m_scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

void PanelPlaylists::clear() {
  for (auto& w : m_album_widgets) {
    w->marked_for_deletion(true);
  }
  m_album_widgets.clear();
}

void PanelPlaylists::show() { is_drawn(true).is_updated(true); }

void PanelPlaylists::hide() {
  m_props.collection_id = std::nullopt;
  is_drawn(false).is_updated(false);
}

void PanelPlaylists::recreate() {
  m_props_old = m_props;
  clear();

  m_panel_search->is_updated(m_props.panel_search_visible).is_drawn(m_props.panel_search_visible);

  m_scrollbar->is_updated(m_props.is_scrollable)
    .is_drawn(m_props.is_scrollable && (m_scrollbar->content_size() > m_scrollbar->page_size()));

  m_button_sort_by->is_updated(m_props.button_sort_by_visible).is_drawn(m_props.button_sort_by_visible);

  std::vector<size_t> playlist_ids_sorted;
  if (m_props.collection_id.has_value()) {
    auto c = db::collection_by_id(*m_props.collection_id);
    playlist_ids_sorted = c->get().playlist_ids();
  } else if (!m_props.playlist_ids.empty()) {
    playlist_ids_sorted = m_props.playlist_ids;
  } else {
    m_width_old = width();
    m_height_old = height();
    reflow();
    return;
  }

  if (m_props.sort_by == SortBy::NAME_AZ) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.name, lhs.author) < std::tie(rhs.name, rhs.author);
    });
  } else if (m_props.sort_by == SortBy::NAME_ZA) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.name, lhs.author) > std::tie(rhs.name, rhs.author);
    });
  } else if (m_props.sort_by == SortBy::AUTHOR_AZ) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.author, lhs.name) < std::tie(rhs.author, rhs.name);
    });
  } else if (m_props.sort_by == SortBy::AUTHOR_ZA) {
    std::sort(playlist_ids_sorted.begin(), playlist_ids_sorted.end(), [](size_t lhs_id, size_t rhs_id) {
      auto& lhs = db::playlist_by_id(lhs_id)->get();
      auto& rhs = db::playlist_by_id(rhs_id)->get();
      return std::tie(lhs.author, lhs.name) > std::tie(rhs.author, rhs.name);
    });
  }

  std::vector<size_t> playlist_ids_sorted_filtered =
    db::search_playlists(m_search_bar->label().text(), playlist_ids_sorted, 512);

  for (size_t playlist_id : playlist_ids_sorted_filtered) {
    auto* album_widget = &m_albums_container->add_child<WidgetAlbumCover>(playlist_id);
    m_album_widgets.emplace_back(album_widget);

    album_widget
      ->on_press([this, playlist_id, album_widget]() {
        if (m_on_playlist_lmb) { m_on_playlist_lmb(playlist_id, album_widget); }
      })
      .on_press_rmb([this, playlist_id, album_widget]() {
        if (m_on_playlist_rmb) { m_on_playlist_rmb(playlist_id, album_widget); }
      });
  }

  if (m_props.collection_id.has_value() && m_props.collection_id == 0 && m_props.button_add_playlist_visible) {
    auto* album_widget = &m_albums_container->add_child<WidgetAlbumCover>(std::nullopt, true);
    m_album_widgets.emplace_back(album_widget);

    album_widget->on_press([this, album_widget]() {
      if (m_on_add_playlist_button_pressed) { m_on_add_playlist_button_pressed(album_widget); }
    });
  }

  m_width_old = width();
  m_height_old = height();
  reflow();
}

void PanelPlaylists::scroll_to_playlist(size_t playlist_id, bool immediate) {
  for (auto& album_widget : m_album_widgets) {
    if (album_widget->playlist_id() == playlist_id) {
      m_target_scroll_px = album_widget->y();
      m_target_scroll_px =
        std::clamp(m_target_scroll_px, 0.0, std::max(0.0, static_cast<double>(m_content_height - height())));
      m_scrollbar->scroll_offset(m_target_scroll_px);
      if (immediate) {
        m_scroll_px = m_target_scroll_px;
        m_scrollbar->skip_anim();
      }
      break;
    }
  }
}

void PanelPlaylists::reflow() {
  m_panel_search->width(width() - (2 * PANEL_SEARCH_PADDING + m_scrollbar->width()));
  m_albums_container->width(m_props.is_scrollable ? width() - m_scrollbar->width() : width());

  auto& config = theme::config().panel_playlists;
  auto& settings = zincbox::settings();
  const float SCALE = zincbox::ui_scale();

  i32 cover_w = settings.interface.cover_size;
  i32 text_space = 0;
  if (config.show_title || config.show_author) { text_space += 8 * SCALE; }
  if (config.show_title) { text_space += 16 * SCALE; }
  if (config.show_author) { text_space += 16 * SCALE; }

  i32 item_w = cover_w;
  i32 item_h = cover_w + text_space;

  i32 min_spacing_x = std::max<i32>(0, config.horizontal_spacing * SCALE);
  i32 spacing_y = config.vertical_spacing * SCALE;

  i32 albums_area_width = m_albums_container->width();
  if (albums_area_width <= 0) {
    m_content_height = 0;
    m_albums_container->height(0);
    return;
  }

  i32 album_covers_in_one_row = (albums_area_width - min_spacing_x) / (item_w + min_spacing_x);
  if (album_covers_in_one_row <= 0) { album_covers_in_one_row = 1; }

  i32 total_items_w = album_covers_in_one_row * item_w;
  i32 total_gap_space = std::max<i32>(0, albums_area_width - total_items_w);
  i32 total_gaps_count = album_covers_in_one_row + 1;

  std::optional<std::string_view> prev_artist;
  std::optional<char32_t> prev_name_first_letter;
  i32 column = 0;
  i32 cover_y = spacing_y;

  for (auto& album_widget : m_album_widgets) {
    bool group_by_author =
      m_props.group && (m_props.sort_by == SortBy::AUTHOR_AZ || m_props.sort_by == SortBy::AUTHOR_ZA);
    bool group_by_name = m_props.group && (m_props.sort_by == SortBy::NAME_AZ || m_props.sort_by == SortBy::NAME_ZA);
    bool force_new_row = false;

    if (group_by_author && prev_artist && album_widget->label_author()->text() != *prev_artist) {
      force_new_row = true;
    }

    std::optional<char32_t> name_first_letter;
    if (!album_widget->label_title()->text().empty()) {
      name_first_letter = transform_to_base_char(album_widget->label_title()->text()[0]);
      if (group_by_name && prev_name_first_letter && *name_first_letter != *prev_name_first_letter) {
        force_new_row = true;
      }
    }

    if (column >= album_covers_in_one_row || (force_new_row && column > 0)) {
      column = 0;
      cover_y += item_h + spacing_y;
    }

    i32 gap_before = (total_gap_space * (column + 1)) / total_gaps_count;
    i32 x_ = gap_before + (column * item_w);
    album_widget->pos(x_, cover_y);

    column += 1;
    prev_artist = album_widget->label_author()->text();
    prev_name_first_letter = name_first_letter;
  }

  i32 covers_total_bounds = m_album_widgets.empty() ? 0 : (cover_y + item_h + spacing_y);
  m_content_height = covers_total_bounds + (m_props.panel_search_visible ? (8 + m_panel_search->height()) : 0);

  m_scrollbar->content_size(m_content_height).page_size(height()).height(height());
  m_scroll_px = std::clamp(m_scroll_px, 0.0, std::max(0.0, static_cast<double>(m_content_height - height())));
  m_albums_container->height(m_content_height);
}

float PanelPlaylists::scroll_px() const {
  if (!m_props.is_scrollable) { return 0.0f; }
  return m_target_scroll_px;
}

void PanelPlaylists::scroll_px(float px, bool immediate) {
  m_scroll_px = px;
  m_target_scroll_px = px;
  m_scrollbar->scroll_offset(px);
  if (immediate) { m_scrollbar->skip_anim(); }
}

vec2i PanelPlaylists::content_size() const { return {width(), m_content_height}; }
