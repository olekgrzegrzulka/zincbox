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
    SpriteAlbumCover(UI& ui_, std::string id, vec2i cover_size_);
};

class WidgetAlbumCover : public Button {
  public:
    WidgetAlbumCover(UI& ui_, std::optional<size_t> playlist_id, vec2i total_size_, vec2i cover_size_);
    void draw() override;
    void update() override;
    void event(Input::InputEventMouseMove& ev) override;

  public:
    const std::optional<size_t> playlist_id = 0;

  protected:
    Sprite* hover{};
    bool is_hovered = false;
    Label* label_title{};
    vec2i total_size{};
    vec2i cover_size{};
};

class PanelAlbums : public Sprite {
  public:
    enum class SortBy : u8 { NAME_AZ, NAME_ZA, AUTHOR_AZ, AUTHOR_ZA };

  public:
    PanelAlbums(UI& ui_);
    void draw() override;
    void clear();
    void update() override;
    void recreate();
    void scroll_to_playlist(size_t);
    using Sprite::event;
    void event(Input::InputEventMouseScroll&) override;
    float get_scroll_px() const;
    void set_scroll_px(float px);
    std::optional<size_t> get_collection_id() const { return props.collection_id; }

  protected:
    void reflow();

  protected:
    double scroll_px = 0.0;
    double target_scroll_px = 0.0;

    std::vector<WidgetAlbumCover*> album_widgets;
    ScrollBar* scrollbar{};

    Sprite* panel_search{};
    Widget* albums_container{};
    TextInput* search_bar{};
    Button* button_clear_search{};
    Button* button_sort_by{};

  public:
    struct Props {
        std::optional<size_t> collection_id{};
        std::vector<size_t> playlist_ids{};
        SortBy sort_by = PanelAlbums::SortBy::AUTHOR_AZ;
        bool panel_search_visible = true;
        bool is_scrollable = true;

        i32 cover_width = 64;
        i32 cover_min_horizontal_spacing = 12;
        i32 cover_min_vertical_spacing = 32;

        bool operator==(const Props&) const = default;
    };

    Props props{};

  protected:
    Props props_old{};

  public:
    std::function<void(size_t, Widget*)> on_playlist_lmb{};
    std::function<void(size_t, Widget*)> on_playlist_rmb{};
    std::function<void(Widget*)> on_button_sort_by_pressed{};
    std::function<void(Widget*)> on_add_playlist_button_pressed{};
};
