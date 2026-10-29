#pragma once
#include "common/signal.hpp"
#include "common/types.hpp"
#include "core/musicdb/types.hpp"
#include "ui/zincgui/button.hpp"

namespace zincgui {
  class Root;
  class Label;
  class ColorRect;
  class Sprite;
  namespace Input {
    struct InputEventMouseMove;
  }
}; // namespace zincgui

class WidgetTrack final : public zincgui::Button {
  public:
    enum class TrackHighlightMode : u8 { TRACK_INFO, QUEUE_INDEX, OFF };

    WidgetTrack(const WidgetTrack&) = delete;
    WidgetTrack(WidgetTrack&&) = delete;
    WidgetTrack& operator=(const WidgetTrack&) = delete;
    WidgetTrack& operator=(WidgetTrack&&) = delete;

    WidgetTrack(zincgui::Root& ui_);
    ~WidgetTrack() override;

    void update() override;
    void event(zincgui::Input::InputEventMouseMove& ev) override;

    void update_highlight_status_from_player();
    void setup();
    void update_text_colors();
    [[nodiscard]] db::track_info track_info() const;

    [[nodiscard]] size_t track_id() const noexcept { return m_track_id; }
    WidgetTrack& track_id(size_t value);

    [[nodiscard]] size_t playlist_id() const noexcept { return m_playlist_id; }
    WidgetTrack& playlist_id(size_t value);

    [[nodiscard]] size_t collection_id() const noexcept { return m_collection_id; }
    WidgetTrack& collection_id(size_t value);

    [[nodiscard]] size_t track_number() const noexcept { return m_track_number; }
    WidgetTrack& track_number(size_t value);

    [[nodiscard]] bool is_selected() const noexcept { return m_is_selected; }
    WidgetTrack& is_selected(bool value);

    [[nodiscard]] size_t item_index() const noexcept { return m_item_index; }
    WidgetTrack& item_index(size_t value);

    [[nodiscard]] TrackHighlightMode highlight_mode() const noexcept { return m_highlight_mode; }
    WidgetTrack& highlight_mode(TrackHighlightMode value);

    [[nodiscard]] bool highlighted() const noexcept { return m_highlighted; }
    WidgetTrack& highlighted(bool highlighted_new);

    [[nodiscard]] bool playback_error() const noexcept { return m_playback_error; }
    WidgetTrack& playback_error(bool playback_error_new);

    [[nodiscard]] zincgui::Label* label_track_number() noexcept { return m_label_track_number; }
    [[nodiscard]] const zincgui::Label* label_track_number() const noexcept { return m_label_track_number; }
    [[nodiscard]] zincgui::Label* label_track_artist() noexcept { return m_label_track_artist; }
    [[nodiscard]] const zincgui::Label* label_track_artist() const noexcept { return m_label_track_artist; }
    [[nodiscard]] zincgui::Label* label_track_title() noexcept { return m_label_track_title; }
    [[nodiscard]] const zincgui::Label* label_track_title() const noexcept { return m_label_track_title; }
    [[nodiscard]] zincgui::Label* label_track_length() noexcept { return m_label_track_length; }
    [[nodiscard]] const zincgui::Label* label_track_length() const noexcept { return m_label_track_length; }
    [[nodiscard]] zincgui::ColorRect* bg() noexcept { return m_bg; }
    [[nodiscard]] const zincgui::ColorRect* bg() const noexcept { return m_bg; }
    [[nodiscard]] zincgui::ColorRect* hover() noexcept { return m_hover; }
    [[nodiscard]] const zincgui::ColorRect* hover() const noexcept { return m_hover; }
    [[nodiscard]] zincgui::Sprite* love_icon() noexcept { return m_love_icon; }
    [[nodiscard]] const zincgui::Sprite* love_icon() const noexcept { return m_love_icon; }

  private:
    bool m_highlighted = false;
    bool m_playback_error = false;
    bool m_pressed = false;
    bool m_is_selected = false;
    size_t m_track_number = 0;
    size_t m_item_index = 0;
    size_t m_track_id = db::INVALID_ID;
    size_t m_playlist_id = db::INVALID_ID;
    size_t m_collection_id = db::INVALID_ID;
    bool m_changed = true;
    TrackHighlightMode m_highlight_mode = TrackHighlightMode::TRACK_INFO;
    Signal<>::slot_key m_slot_on_track_changed;
    zincgui::Label* m_label_track_number{};
    zincgui::Label* m_label_track_artist{};
    zincgui::Label* m_label_track_title{};
    zincgui::Label* m_label_track_length{};
    zincgui::ColorRect* m_bg{};
    zincgui::ColorRect* m_hover{};
    zincgui::Sprite* m_love_icon{};
};
