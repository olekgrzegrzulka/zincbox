#include <cassert>
#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include "common/input.hpp"
#include "core/musicdb.hpp"
#include "panel_albums.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/panel.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

SpriteAlbumCover::SpriteAlbumCover(UI& ui_, std::string id, TextureAtlas* album_covers_atlas_) : Sprite(ui_) {
  if (album_covers_atlas_) {
    album_covers_atlas = album_covers_atlas_;
  } else {
    album_covers_atlas = &ui.get_texture_atlas();
  }
  set_texture(id);
  set_nine_slice_margin(0);
}

WidgetAlbumCover::WidgetAlbumCover(UI& ui_, size_t playlist_id, TextureAtlas* album_covers_atlas_) : Button(ui_) {
  if (album_covers_atlas_) {
    album_covers_atlas = album_covers_atlas_;
  } else {
    album_covers_atlas = &ui.get_texture_atlas();
  }
  auto& playlist = db::playlist_by_id(playlist_id)->get();
  get_children()[0]->set_ignore_parents_layout(true);
  set_clip_children(true);
  set_size(COVER_WIDTH, COVER_HEIGHT);
  set_layout("m:0 s:8 ttb");
  std::stringstream texture_id;
  // texture_id << album->album_id;
  auto& sprite_cover = add_child<SpriteAlbumCover>(std::to_string(playlist_id), album_covers_atlas);
  label_title = &add_child<Label>(playlist.name);
  label_title->set_resize_to_text_extents(false);
  label_title->set_width(COVER_WIDTH);
  label_title->set_label_anchor(Anchor::LEFT);
  label_title->set_anchor(Anchor::LEFT);
  label_title->set_parent_anchor(Anchor::LEFT);
  label_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.65f);

  hover = &sprite_cover.add_child<Sprite>("playlist_hovered");
  hover->set_nine_slice_margin(4.0f);
  hover->set_ignore_parents_layout(true);
  hover->set_size(64, 64);
}

void WidgetAlbumCover::draw() {
  // Button::draw();
}

void WidgetAlbumCover::update() {
  if (!is_mouse_hovering()) { is_hovered = false; }

  hover->set_is_drawn(is_hovered);

  if (is_hovered && label_title->get_text_extents().x > COVER_WIDTH + 8) {
    label_title->set_x(label_title->get_x() - 1);
    if (label_title->get_x() <= -label_title->get_text_extents().x - 8) {
      label_title->set_x(COVER_WIDTH + 8);
    }
  } else {
    label_title->set_x(0);
  }
  Button::update();
}

void WidgetAlbumCover::handle_event(Input::InputEventMouseMove& ev) {
  is_hovered = is_mouse_hovering();
  Button::handle_event(ev);
}

PanelAlbums::PanelAlbums(UI& ui_) : Panel(ui_, Panel::PanelStyle::RectangularDark, false) {
  set_clip_children(true);

  scrollbar = &add_child<ScrollBar>();
  scrollbar->set_anchor(Anchor::RIGHT);
  scrollbar->set_parent_anchor(Anchor::RIGHT);
  scrollbar->set_width(12);
  scrollbar->set_thumb_thickness(12);
  scrollbar->set_orientation(SliderOrientation::VERTICAL);
  scrollbar->set_track_thickness(12);
  scrollbar->on_value_changed([&](i32 /* old */, i32 scroll_offset) {
    target_scroll_px = scroll_offset;
  });

  albums_container = &add_child<Widget>();
  albums_container->set_pos(0, 4 + 36 + 4);

  panel_top = &add_child<Panel>(PanelStyle::Rounded);
  panel_top->set_pos(4, 4);
  panel_top->set_height(36);
  panel_top->set_layout("m:6 s:6 rtl fill expand");
  panel_top->set_ignore_parents_layout(true);
  button_sort_by = &panel_top->add_child<Button>();
  button_group_by = &panel_top->add_child<Button>();
  search_bar = &panel_top->add_child<TextInput>();
  button_clear_search = &search_bar->add_child<Button>();

  auto btn_icons = std::to_array({"sort_by", "group_by"});
  auto buttons = std::to_array({button_sort_by, button_group_by});
  for (size_t i = 0; i < 2; i += 1) {
    buttons[i]->set_max_width(24);
    buttons[i]->set_max_height(24);
    auto& img = buttons[i]->add_child<Sprite>(btn_icons[i]);
    img.set_parent_anchor(Anchor::CENTER);
    img.set_anchor(Anchor::CENTER);
  }

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

  button_clear_search->on_press([this]() {
    this->search_bar->clear();
  });
}

void PanelAlbums::draw() {
  // album_covers_atlas.bind(0);
  Panel::draw();
  // ui.get_texture_atlas().bind(0);
}

void PanelAlbums::clear() {
  for (auto& w : album_widgets) {
    w->set_marked_for_deletion(true);
  }
  album_widgets.clear();
}

void PanelAlbums::recreate(std::optional<size_t> collection_id_, TextureAtlas* album_covers_atlas) {
  clear();
  if (!collection_id_.has_value() || album_covers_atlas == nullptr) { return; }

  auto c = db::collection_by_id(*collection_id_);

  for (size_t playlist_id : c->get().playlist_ids) {
    if (db::playlist_by_id(playlist_id)->get().is_tombstone()) { continue; }
    
    auto* album_widget = &albums_container->add_child<WidgetAlbumCover>(playlist_id, album_covers_atlas);
    album_widgets.emplace_back(album_widget);
    album_widget->on_press([this, playlist_id, album_widget]() {
      if (on_playlist_lmb) {
        on_playlist_lmb(playlist_id, album_widget);
      }
    });
    album_widget->on_press_rmb([this, playlist_id, album_widget]() {
      if (on_playlist_rmb) {
        on_playlist_rmb(playlist_id, album_widget);
      }
    });
  }
}

void PanelAlbums::update() {
  panel_top->set_width(width - 4 - 4 - scrollbar->get_width());
  albums_container->set_width(width - 4 - 4 - scrollbar->get_width());
  albums_container->set_height(height - (4 + scrollbar->get_height() + 4));

  i32 albums_area_width = get_width() - scrollbar->get_width();
  i32 album_covers_in_one_row = get_width() / COVER_WIDTH;

  if (album_covers_in_one_row > 0) {
    i32 i = 0;
    for (auto& cover : album_widgets) {
      if (!cover->get_is_updated()) { continue; }

      i32 x = (i % album_covers_in_one_row) * (albums_area_width) / album_covers_in_one_row;
      i32 y = ((i32)(i / album_covers_in_one_row) * (COVER_HEIGHT)) - scroll_px;
      cover->set_pos(x, y);

      i += 1;
    }

    i32 row_count = (album_widgets.size() + album_covers_in_one_row - 1) / album_covers_in_one_row;
    i32 content_size = row_count * COVER_HEIGHT + (8 + panel_top->get_height());
    scrollbar->set_content_size(content_size);
    scrollbar->set_page_size(height);
    scrollbar->set_height(height);
    scrollbar->set_height(height);
    scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(content_size - height)));
  }

  double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.4, 0.8);
  scroll_px = std::lerp(scroll_px, target_scroll_px, t);

  Panel::update();
}

void PanelAlbums::handle_event(Input::InputEventMouseScroll& e) {
  if (is_mouse_hovering()) {
    scrollbar->scroll(e.offset.y);
    e.handled = true;
  }
}

float PanelAlbums::get_scroll_px() const {
  return target_scroll_px;
}

void PanelAlbums::set_scroll_px(float px) {
  scroll_px = px;
  target_scroll_px = px;
  scrollbar->set_scroll_offset(px);
}
