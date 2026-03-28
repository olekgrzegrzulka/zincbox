#pragma once
#include <sstream>
#include <string>
#include "core/musicdb.hpp"
#include "core/player.hpp"
#include "debug.hpp"
#include "theme.hpp"
#include "types.hpp"
#include "ui/button.hpp"
#include "ui/panel.hpp"
#include "ui/ui.hpp"

class WidgetTrack : public Button {
  public:
    WidgetTrack(UI& ui_, size_t collection_id_, size_t album_id_, size_t track_id_, bool even_) : Button(ui_) {
      track_id = track_id_;
      album_id = album_id_;
      collection_id = collection_id_;
      even = even_;
      auto& track = db::track_by_id(track_id)->get();
      std::string txt = even ? "track_bg2" : "track_bg1";
      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      set_texture(txt, false);
      set_nine_slice_margin(4.0);
      set_height(TRACK_HEIGHT);

      std::stringstream track_number;
      track_number << track.track_number;
      label_track_number = &add_child<Label>(track_number.str());
      label_track_number->set_resize_to_text_extents(false);
      label_track_number->set_label_anchor(Anchor::LEFT);
      label_track_number->set_size(20, TRACK_HEIGHT);
      label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
      label_track_number->set_x(5);

      label_track_artist = &add_child<Label>(track.artist);
      label_track_artist->set_resize_to_text_extents(false);
      label_track_artist->set_label_anchor(Anchor::LEFT);
      label_track_artist->set_height(TRACK_HEIGHT);
      label_track_artist->set_x(label_track_number->get_x() + label_track_number->get_width() + 5);
      label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);

      label_track_title = &add_child<Label>(track.title);
      label_track_title->set_resize_to_text_extents(false);
      label_track_title->set_x(label_track_artist->get_x() + label_track_artist->get_width() + 5);
      label_track_title->set_label_anchor(Anchor::LEFT);
      label_track_title->set_height(TRACK_HEIGHT);
      label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);

      panel_right_side = &add_child<Panel>();
      panel_right_side->set_height(TRACK_HEIGHT);
      panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
      panel_right_side->set_layout("m:8 s:8 rtl expand fit");
      panel_right_side->set_anchor(Anchor::RIGHT);
      panel_right_side->set_parent_anchor(Anchor::RIGHT);
      panel_right_side->set_ignore_parents_layout(true);

      i32 length_s = track.length_seconds;
      i32 length_m = length_s / 60;
      length_s %= 60;
      std::stringstream ss;
      ss << std::right << std::setfill('0') << std::setw(0) << length_m << ":" << std::setw(2) << length_s;
      label_track_length = &panel_right_side->add_child<Label>(ss.str());
      label_track_length->set_height(TRACK_HEIGHT);
      label_track_length->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
      label_track_length->set_resize_to_text_extents(false);
    }

    void update() override {
      playing = (player::get_playing() &&
                 track_id == player::get_playing()->track_id &&
                 album_id == player::get_playing()->playlist_id &&
                 collection_id == player::get_playing()->collection_id);

      if (playing != playing_old && playing) {
        panel_right_side->set_texture("track_bg_playing", false);
        std::string txt = "track_bg_playing";
        set_texture_idle(txt);
        set_texture_hovered(txt);
        set_texture_disabled(txt);
        set_texture_pressed(txt);
        set_texture(txt, false);
        label_track_number->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.2f);
        label_track_artist->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 0.9f);
        label_track_title->set_text_color(glm::vec3{0.55, 0.35, 0.45} * 1.5f);
      }
      if (playing != playing_old && !playing) {
        panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
        std::string txt = even ? "track_bg2" : "track_bg1";
        set_texture_idle(txt);
        set_texture_hovered(txt);
        set_texture_disabled(txt);
        set_texture_pressed(txt);
        set_texture(txt, false);
        label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
        label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);
        label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);
      }

      playing_old = playing;

      Sprite::update();
    }

    // void handle_event(Input::InputEventMouseButton& ev) override {
    //   if (is_mouse_hovering() && ev.button == Input::MouseButton::MOUSE_BUTTON_LEFT) {
    //     if (ev.action == Input::MouseAction::PRESS) {
    //       pressed = true;
    //     }
    //     if (ev.action == Input::MouseAction::RELEASE && pressed) {
    //       player::now_playing_t now_playing{
    //         .track_id = track_id,
    //         .album_id = album_id,
    //         .collection_id = collection_id,
    //         .playlist_track = playlist_track,
    //       };
    //       player::play(now_playing, track_id);
    //     }
    //     ev.handled = true;
    //   }
    // }

  protected:
    Label* label_track_number{};
    Label* label_track_artist{};
    Label* label_track_title{};
    Panel* panel_right_side{};
    Label* label_track_length{};
    bool even = false;
    bool playing = false;
    bool playing_old = false;
    size_t collection_id{};
    size_t album_id{};
    size_t track_id{};
    std::optional<size_t> playlist_track{};
    bool pressed = false;
};
