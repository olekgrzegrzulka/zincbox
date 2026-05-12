#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "common/input.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/scrollbar.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/text_input.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class SpriteAlbumCover : public Sprite {
  public:
    SpriteAlbumCover(UI& ui_, std::string id);
};

class WidgetAlbumCover : public Button {
  public:
    WidgetAlbumCover(UI& ui_, std::optional<size_t> playlist_id = std::nullopt);
    void draw() override;
    void update() override;
    void handle_event(Input::InputEventMouseMove& ev) override;

  public:
    const std::optional<size_t> playlist_id = 0;

  protected:
    Sprite* hover{};
    bool is_hovered = false;
    Label* label_title{};
};

class PanelAlbums : public Sprite {
  public:
    enum class SortBy {
      NAME_AZ,
      NAME_ZA,
      AUTHOR_AZ,
      AUTHOR_ZA,
    };

  public:
    PanelAlbums(UI& ui_);
    void draw() override;
    void clear();
    void recreate(std::optional<size_t> collection_id_, SortBy sort_by_ = SortBy::AUTHOR_AZ);
    void update() override;
    using Sprite::handle_event;
    void handle_event(Input::InputEventMouseScroll&) override;
    float get_scroll_px() const;
    void set_scroll_px(float px);
    std::optional<size_t> get_collection_id() const { return collection_id; }

  protected:
    void reflow();

  protected:
    std::optional<size_t> collection_id{};
    double scroll_px = 0.0;
    double target_scroll_px = 0.0;
    SortBy sort_by = PanelAlbums::SortBy::AUTHOR_AZ;
    std::vector<WidgetAlbumCover*> album_widgets;
    ScrollBar* scrollbar{};

    Sprite* panel_top{};
    Widget* albums_container{};
    TextInput* search_bar{};
    Button* button_clear_search{};
    Button* button_sort_by{};

  public:
    std::function<void(size_t, Widget*)> on_playlist_lmb{};
    std::function<void(size_t, Widget*)> on_playlist_rmb{};
    std::function<void(Widget*)> on_button_sort_by_pressed{};
    std::function<void()> on_search_bar_text_modified{};
    std::function<void(Widget*)> on_add_playlist_button_pressed{};
};
