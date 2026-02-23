#pragma once
#include <cassert>
#include "debug.hpp"
#include "musicdb.hpp"
#include "texture_atlas.hpp"
#include "ui/button.hpp"
#include "ui/panel.hpp"
#include "ui/ui.hpp"
#include "ui/widget.hpp"

static constexpr i32 COVER_WIDTH = 64 + 4;
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
    // set_is_self_drawn(false);s
    std::string sprite_id = album_covers_atlas.has_texture(album->id) ? album->id : "cover_unknown";
    auto& sprite_cover = add_child<SpriteAlbumCover>(sprite_id, album_covers_atlas);
    label_title = &add_child<Label>();
    label_title->set_text(album->title);
    label_title->set_width(64);
    label_title->set_height(label_title->get_text_extents().y);
    label_title->set_label_anchor(Anchor::LEFT);
    label_title->set_text_color({0.75, 0.75, 0.75});
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
  PanelAlbums(UI& ui_) : Panel(ui_, Panel::PanelStyle::RoundedDark, false) {
  }

  void draw() override {
    // album_covers_atlas.bind(0);
    Panel::draw();
    // ui.get_texture_atlas().bind(0);
  }

  void recreate() {
    album_covers_atlas = TextureAtlas{};
    album_covers_atlas.add_texture("cover_unknown", "./assets/cover_unknown.png");

    for (auto& album : musicdb::get_albums()) {
      album_covers_atlas.add_texture(album.id, album.cover_art, 64, 64);
    }

    ensure(album_covers_atlas.get("0").has_value());

    album_covers_atlas.save_to_file("albums.png");

    for (auto& c : get_children()) {
      c->set_marked_for_deletion(true);
    }

    i32 id = 0;
    for (auto& album : musicdb::get_albums()) {
      auto& album_widget = add_child<WidgetAlbumCover>(&album, album_covers_atlas);
      album_widget.on_press([=]() { debug_log(id); });
      id += 1;
    }
  }

  void update() override {
    i32 album_covers_in_one_row = get_width() / COVER_WIDTH;
    if (album_covers_in_one_row > 0) {
      i32 i = 0;
      for (auto& cover : get_children()) {
        i32 x = (i % album_covers_in_one_row) * (get_width()) / album_covers_in_one_row;
        i32 y = (i / album_covers_in_one_row) * (COVER_HEIGHT);
        cover->set_pos(x, y);

        i += 1;
      }
    }

    Panel::update();
  }

protected:
  TextureAtlas album_covers_atlas;
};
