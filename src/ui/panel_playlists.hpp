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
    SpriteAlbumCover(zincgui::Root& ui_, std::optional<db::playlist_id_t> playlist_id, vec2i cover_size_,
                     bool is_add_button);

    zincgui::TextureAtlas& texture_atlas() override;

    bool cover(std::optional<db::playlist_id_t> playlist_id);
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

    [[nodiscard]] bool is_add_button() const noexcept { return m_is_add_button; }
    [[nodiscard]] std::optional<size_t> playlist_id() const noexcept { return m_playlist_id; }
    [[nodiscard]] const zincgui::Sprite* hover() const noexcept { return m_hover; }
    [[nodiscard]] zincgui::Sprite* hover() noexcept { return m_hover; }
    [[nodiscard]] const zincgui::Sprite* sprite_playing() const noexcept { return m_sprite_playing; }
    [[nodiscard]] zincgui::Sprite* sprite_playing() noexcept { return m_sprite_playing; }
    [[nodiscard]] const zincgui::Label* label_title() const noexcept { return m_label_title; }
    [[nodiscard]] zincgui::Label* label_title() noexcept { return m_label_title; }
    [[nodiscard]] const zincgui::Label* label_author() const noexcept { return m_label_author; }
    [[nodiscard]] zincgui::Label* label_author() noexcept { return m_label_author; }
    [[nodiscard]] vec2i total_size() const noexcept { return m_total_size; }
    [[nodiscard]] vec2i cover_size() const noexcept { return m_cover_size; }

  protected:
    void update_highlight_status_from_player();

  private:
    std::optional<size_t> m_playlist_id = 0;
    bool m_is_add_button = false;
    zincgui::Sprite* m_hover{};
    zincgui::Sprite* m_sprite_playing{};
    zincgui::Label* m_label_title{};
    zincgui::Label* m_label_author{};
    vec2i m_total_size{};
    vec2i m_cover_size{};
    Signal<>::slot_key m_slot_on_track_changed;
};

class PanelPlaylists final : public zincgui::ColorRect {
  public:
    enum class SortBy : u8 { NAME_AZ, NAME_ZA, AUTHOR_AZ, AUTHOR_ZA };

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

    PanelPlaylists(zincgui::Root& ui_);

    using ColorRect::event;
    void input() override;
    void update() override;
    void event(zincgui::Input::InputEventMouseScroll& e) override;

    void clear();
    void show();
    void hide();
    void recreate();
    void scroll_to_playlist(size_t playlist_id, bool immediate = false);
    void reflow();

    [[nodiscard]] float scroll_px() const;
    void scroll_px(float px, bool immediate = false);

    [[nodiscard]] vec2i content_size() const;

    [[nodiscard]] std::optional<size_t> collection_id() const noexcept { return m_props.collection_id; }

    [[nodiscard]] const Props& props() const noexcept { return m_props; }
    [[nodiscard]] Props& props() noexcept { return m_props; }
    PanelPlaylists& props(const Props& p) noexcept {
      m_props = p;
      return *this;
    }

    PanelPlaylists& on_playlist_lmb(std::function<void(size_t, Widget*)> cb) {
      m_on_playlist_lmb = std::move(cb);
      return *this;
    }
    PanelPlaylists& on_playlist_rmb(std::function<void(size_t, Widget*)> cb) {
      m_on_playlist_rmb = std::move(cb);
      return *this;
    }
    PanelPlaylists& on_button_sort_by_pressed(std::function<void(Widget*)> cb) {
      m_on_button_sort_by_pressed = std::move(cb);
      return *this;
    }
    PanelPlaylists& on_add_playlist_button_pressed(std::function<void(Widget*)> cb) {
      m_on_add_playlist_button_pressed = std::move(cb);
      return *this;
    }

    [[nodiscard]] const std::vector<WidgetAlbumCover*>& album_widgets() const noexcept { return m_album_widgets; }
    [[nodiscard]] zincgui::ScrollBar* scrollbar() noexcept { return m_scrollbar; }
    [[nodiscard]] const zincgui::ScrollBar* scrollbar() const noexcept { return m_scrollbar; }
    [[nodiscard]] zincgui::Sprite* panel_search() noexcept { return m_panel_search; }
    [[nodiscard]] const zincgui::Sprite* panel_search() const noexcept { return m_panel_search; }
    [[nodiscard]] zincgui::Widget* albums_container() noexcept { return m_albums_container; }
    [[nodiscard]] const zincgui::Widget* albums_container() const noexcept { return m_albums_container; }
    [[nodiscard]] zincgui::TextInput* search_bar() noexcept { return m_search_bar; }
    [[nodiscard]] const zincgui::TextInput* search_bar() const noexcept { return m_search_bar; }
    [[nodiscard]] zincgui::Button* button_clear_search() noexcept { return m_button_clear_search; }
    [[nodiscard]] const zincgui::Button* button_clear_search() const noexcept { return m_button_clear_search; }
    [[nodiscard]] zincgui::Button* button_sort_by() noexcept { return m_button_sort_by; }
    [[nodiscard]] const zincgui::Button* button_sort_by() const noexcept { return m_button_sort_by; }

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

    Props m_props{};
    Props m_props_old{};
    i32 m_width_old{};
    i32 m_height_old{};

    std::function<void(size_t, Widget*)> m_on_playlist_lmb;
    std::function<void(size_t, Widget*)> m_on_playlist_rmb;
    std::function<void(Widget*)> m_on_button_sort_by_pressed;
    std::function<void(Widget*)> m_on_add_playlist_button_pressed;
};
