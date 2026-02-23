#pragma once
#include <cassert>
#include <sstream>
#include <vector>
#include "bridge.hpp"
#include "debug.hpp"
#include "input.hpp"
#include "musicdb.hpp"
#include "opengl_includes.hpp"
#include "texture_atlas.hpp"
#include "ui/button.hpp"
#include "ui/panel.hpp"
#include "ui/scrollbar.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

static constexpr i32 COVER_WIDTH = 64 + 12;
static constexpr i32 COVER_HEIGHT = 64 + 32;

class SpriteAlbumCover : public Sprite {
public:
  SpriteAlbumCover(UI& ui_, std::string id, TextureAtlas& album_covers_atlas_) : Sprite(ui_), album_covers_atlas(album_covers_atlas_) {
    set_texture(id);
    set_nine_slice_margin(0);
  }

  TextureAtlas& get_texture_atlas() override {
    return album_covers_atlas;
  }

protected:
  TextureAtlas& album_covers_atlas;
};

class WidgetAlbumCover : public Button {
public:
  WidgetAlbumCover(UI& ui_, const musicdb::Album* album, TextureAtlas& album_covers_atlas_) : Button(ui_), album_covers_atlas(album_covers_atlas_) {
    get_children()[0]->set_ignore_parents_layout(true);
    set_clip_children(true);
    set_size(COVER_WIDTH, COVER_HEIGHT);
    set_layout("m:0 s:8 ttb");
    std::stringstream texture_id;
    texture_id << album->id;
    std::string sprite_id = album_covers_atlas.has_texture(texture_id.str()) ? texture_id.str() : "cover_unknown";
    auto& sprite_cover = add_child<SpriteAlbumCover>(sprite_id, album_covers_atlas);
    label_title = &add_child<Label>();
    label_title->set_text(album->title);
    label_title->set_width(COVER_WIDTH);
    label_title->set_height(label_title->get_text_extents().y);
    label_title->set_label_anchor(Anchor::LEFT);
    label_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.65f);
  }

  void draw() override {
    Widget::draw();
  }

  void update() override {
    if (is_mouse_hovering() && label_title->get_text_extents().x > COVER_WIDTH + 8) {
      label_title->set_x(label_title->get_x() - 1);
      if (label_title->get_x() <= -label_title->get_text_extents().x - 8) {
        label_title->set_x(COVER_WIDTH + 8);
      }
    } else {
      label_title->set_x(0);
    }
    Widget::update();
  }

protected:
  Label* label_title{};
  TextureAtlas& album_covers_atlas;
};

class PanelAlbums : public Panel {
public:
  PanelAlbums(UI& ui_) : Panel(ui_, Panel::PanelStyle::RectangularDark, false) {
    set_clip_children(true);

    scrollbar = &add_child<ScrollBar>();
    scrollbar->set_anchor(Anchor::RIGHT);
    scrollbar->set_parent_anchor(Anchor::RIGHT);
    scrollbar->set_ignore_parents_layout(true);
    scrollbar->on_scroll_offset_changed([&](i32 scroll_offset) {
      target_scroll_px = scroll_offset;
    });
  }

  void draw() override {
    // album_covers_atlas.bind(0);
    Panel::draw();
    // ui.get_texture_atlas().bind(0);
  }

  void recreate() {
    i32 album_count = musicdb::get_albums().size();
    i32 atlas_resolution = std::sqrt(album_count) * 64;
    if (atlas_resolution < 512) {
      atlas_resolution = 512;
    } else if (atlas_resolution < 1024) {
      atlas_resolution = 1024;
    } else if (atlas_resolution < 2048) {
      atlas_resolution = 2048;
    } else {
      atlas_resolution = 2048;
      debug_warn("album_count = ", album_count, ", not supported!");
    }
    album_covers_atlas = TextureAtlas{atlas_resolution, 0, 64};
    album_covers_atlas.add_texture("cover_unknown", "./assets/cover_unknown.png");

    i32 i = 0;
    for (auto& album : musicdb::get_albums()) {
      std::stringstream texture_id;
      texture_id << album.id;
      album_covers_atlas.add_texture(texture_id.str(), album.cover_art, 64, 64);
      if (i++ >= 2048) { break; }
    }

    album_covers_atlas.save_to_file("albums.png");

    for (auto& c : album_widgets) {
      c->set_marked_for_deletion(true);
    }
    album_widgets.clear();

    i32 id = 0;
    for (auto& album : musicdb::get_albums()) {
      auto& album_widget = add_child<WidgetAlbumCover>(&album, album_covers_atlas);
      album_widgets.emplace_back(&album_widget);
      album_widget.on_press([=]() { bridge::on_album_clicked(&album); });
      id += 1;
    }
  }

  void update() override {
    i32 albums_area_width = get_width() - scrollbar->get_width();
    i32 album_covers_in_one_row = get_width() / COVER_WIDTH;

    if (album_covers_in_one_row > 0) {
      i32 i = 0;
      for (auto& cover : album_widgets) {
        i32 x = (i % album_covers_in_one_row) * (albums_area_width) / album_covers_in_one_row;
        i32 y = ((i32)(i / album_covers_in_one_row) * (COVER_HEIGHT)) - scroll_px;
        cover->set_pos(x, y);

        i += 1;
      }

      i32 row_count = (album_widgets.size() + album_covers_in_one_row - 1) / album_covers_in_one_row;
      i32 content_size = row_count * COVER_HEIGHT;
      scrollbar->set_content_size(content_size);
      scrollbar->set_page_size(height);
      scrollbar->set_height(height);
      scrollbar->set_is_drawn(content_size > height);
      scroll_px = std::clamp(scroll_px, 0.0, std::max(0.0, (double)(content_size - height)));
    }

    double t = std::clamp(std::abs(scroll_px - target_scroll_px) * 0.004, 0.3, 0.75);
    scroll_px = std::lerp(scroll_px, target_scroll_px, t);

    Panel::update();
  }

  void handle_event(Input::InputEventMouseScroll& e) override {
    if (is_mouse_hovering()) {
      scrollbar->scroll(e.offset.y);
      e.handled = true;
    }
  }

protected:
  double scroll_px = 0.0;
  double target_scroll_px = 0.0;
  std::vector<WidgetAlbumCover*> album_widgets;
  TextureAtlas album_covers_atlas;
  ScrollBar* scrollbar{};
};
