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
    void set_highlighted(bool highlighted_new);
    void set_playback_error(bool playback_error_new);
    void update_text_colors();

    WidgetTrack& track_id(size_t value);
    WidgetTrack& playlist_id(size_t value);
    WidgetTrack& collection_id(size_t value);
    WidgetTrack& track_number(size_t value);
    WidgetTrack& is_selected(bool value);
    WidgetTrack& item_index(size_t value);
    WidgetTrack& highlight_mode(TrackHighlightMode value);

    size_t track_id() const;
    size_t playlist_id() const;
    size_t collection_id() const;
    size_t track_number() const;
    bool is_selected() const;
    size_t item_index() const;
    db::track_info track_info() const;

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
