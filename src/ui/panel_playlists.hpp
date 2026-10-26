#pragma once
#include <functional>
#include <optional>
#include <vector>
#include <stddef.h>
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/sprite.hpp"

namespace zincgui {
  class Label;
  class ScrollBar;
  class TextInput;
  class Root;
  class Widget;
} // namespace zincgui

class SpriteAlbumCover : public zincgui::Sprite {
  public:
    SpriteAlbumCover(zincgui::Root& ui_, std::optional<db::playlist_id_t>, vec2i cover_size_, bool is_add_button);
    bool set_cover(std::optional<db::playlist_id_t>);
    zincgui::TextureAtlas& get_texture_atlas() override;
};

class WidgetAlbumCover final : public zincgui::Button {
  public:
    WidgetAlbumCover(const WidgetAlbumCover&) = delete;
    WidgetAlbumCover(WidgetAlbumCover&&) = delete;
    WidgetAlbumCover& operator=(const WidgetAlbumCover&) = delete;
    WidgetAlbumCover& operator=(WidgetAlbumCover&&) = delete;
    WidgetAlbumCover(zincgui::Root& ui_, std::optional<size_t> playlist_id, bool is_add_button_ = false);
    ~WidgetAlbumCover() override;
    void update() override;
    void event(zincgui::Input::InputEventMouseMove& ev) override;
    bool is_add_button() const { return m_is_add_button; }

  protected:
    void update_highlight_status_from_player();

  public:
    const std::optional<size_t> playlist_id = 0;
    zincgui::Sprite* hover{};
    zincgui::Sprite* sprite_playing{};
    bool is_hovered = false;
    zincgui::Label* label_title{};
    zincgui::Label* label_author{};
    vec2i total_size{};
    vec2i cover_size{};

  private:
    bool m_is_add_button = false;
    Signal<>::slot_key m_slot_on_track_changed;
};

class PanelPlaylists final : public zincgui::ColorRect {
  public:
    enum class SortBy : u8 { NAME_AZ, NAME_ZA, AUTHOR_AZ, AUTHOR_ZA };

  public:
    PanelPlaylists(zincgui::Root& ui_);
    void clear();
    void input() override;
    void update() override;
    void show();
    void hide();
    void recreate();
    void scroll_to_playlist(size_t, bool immediate = false);
    using ColorRect::event;
    void event(zincgui::Input::InputEventMouseScroll&) override;
    float get_scroll_px() const;
    void set_scroll_px(float px, bool immediate = false);
    vec2i get_content_size() const;
    std::optional<size_t> get_collection_id() const { return props.collection_id; }
    void reflow();

  private:
    static constexpr i32 PANEL_SEARCH_PADDING = 4;
    double m_scroll_px = 0.0;
    double m_target_scroll_px = 0.0;
    i32 m_content_height = 0;

    std::vector<WidgetAlbumCover*> m_album_widgets;
    zincgui::ScrollBar* m_scrollbar{};

    zincgui::Sprite* m_panel_search{};
    zincgui::Widget* m_albums_container{};
    zincgui::TextInput* m_search_bar{};
    zincgui::Button* m_button_clear_search{};
    zincgui::Button* m_button_sort_by{};

  public:
    struct Props {
        std::optional<size_t> collection_id;
        std::vector<size_t> playlist_ids;
        SortBy sort_by = PanelPlaylists::SortBy::AUTHOR_AZ;
        bool group = false;
        bool panel_search_visible = true;
        bool button_sort_by_visible = true;
        bool button_add_playlist_visible = true;
        bool is_scrollable = true;

        bool operator==(const Props&) const = default;
    };

    Props props{};

  private:
    Props m_props_old{};

  public:
    std::function<void(size_t, Widget*)> on_playlist_lmb;
    std::function<void(size_t, Widget*)> on_playlist_rmb;
    std::function<void(Widget*)> on_button_sort_by_pressed;
    std::function<void(Widget*)> on_add_playlist_button_pressed;
};
