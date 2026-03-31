#pragma once
#include <functional>
#include <vector>
#include "common/input.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/panel.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/texture_atlas.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class SpriteAlbumCover : public Sprite {
  public:
    SpriteAlbumCover(UI& ui_, std::string id, TextureAtlas*);
    TextureAtlas& get_texture_atlas() override { return *album_covers_atlas; }

  protected:
    TextureAtlas* album_covers_atlas;
};

class WidgetAlbumCover : public Button {
  public:
    WidgetAlbumCover(UI& ui_, size_t playlist_id, TextureAtlas* album_covers_atlas_);
    void draw() override;
    void update() override;

  protected:
    Label* label_title{};
    TextureAtlas* album_covers_atlas;
};

class PanelAlbums : public Panel {
  public:
    PanelAlbums(UI& ui_);
    void draw() override;
    void recreate(std::optional<size_t> collection_id_, TextureAtlas* album_covers_atlas);
    void update() override;
    using Panel::handle_event;
    void handle_event(Input::InputEventMouseScroll&) override;

  protected:
    double scroll_px = 0.0;
    double target_scroll_px = 0.0;
    std::vector<WidgetAlbumCover*> album_widgets;
    ScrollBar* scrollbar{};

  public:
    std::function<void(size_t, Widget*)> on_playlist_lmb{};
    std::function<void(size_t, Widget*)> on_playlist_rmb{};
};
