#pragma once
#include <sstream>
#include <string>
#include "common/debug.hpp"
#include "common/types.hpp"
#include "core/musicdb.hpp"
#include "core/utf.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/panel.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"

class WidgetTrack : public Button {
  public:
    WidgetTrack(UI& ui_, size_t collection_id_, size_t album_id_, size_t track_id_, size_t track_number, bool even_) : Button(ui_) {
      collection_id = collection_id_;
      album_id = album_id_;
      track_id = track_id_;
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

      label_track_number = &add_child<Label>(std::to_string(track_number));
      label_track_number->set_resize_to_text_extents(false);
      label_track_number->set_label_anchor(Anchor::LEFT);
      label_track_number->set_size(20, TRACK_HEIGHT);
      label_track_number->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.2f);
      label_track_number->set_x(5);

      if (track.artist.empty() || track.title.empty()) {
        label_track_artist = &add_child<Label>();
        label_track_artist->set_resize_to_text_extents(false);
        label_track_artist->set_width(0);
      } else {
        label_track_artist = &add_child<Label>(track.artist);
      }
      label_track_artist->set_resize_to_text_extents(false);
      label_track_artist->set_label_anchor(Anchor::LEFT);
      label_track_artist->set_height(TRACK_HEIGHT);
      label_track_artist->set_x(label_track_number->get_x() + label_track_number->get_width() + 5);
      label_track_artist->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 0.9f);

      if (track.artist.empty() || track.title.empty()) {
        label_track_title = &add_child<Label>(std::filesystem::path{track.path}.filename().string());
      } else {
        label_track_title = &add_child<Label>(track.title);
      }
      label_track_title->set_resize_to_text_extents(false);
      label_track_title->set_x(label_track_artist->get_x() + label_track_artist->get_width() + 5);
      label_track_title->set_label_anchor(Anchor::LEFT);
      label_track_title->set_height(TRACK_HEIGHT);
      label_track_title->set_text_color(glm::vec3{0.50, 0.40, 0.48} * 1.5f);

      panel_right_side = &add_child<Panel>();
      panel_right_side->set_height(TRACK_HEIGHT);
      panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
      panel_right_side->set_layout("m:8 s:8 rtl fit");
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

      love_icon = &panel_right_side->add_child<Sprite>("love");
      love_icon->set_size(12, 12);
      love_icon->set_nine_slice_margin(0);
      love_icon->set_is_drawn(db::playlist_loved_tracks().has_track_id(track_id_));

      if (track.is_tombstone()) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      }

      hover = &add_child<Sprite>("track_hovered");
    }

    void setup(size_t collection_id_, size_t album_id_, size_t track_id_, size_t track_number, bool even_) {
      collection_id = collection_id_;
      album_id = album_id_;
      track_id = track_id_;
      even = even_;

      auto& track = db::track_by_id(track_id)->get();
      if (track.artist.empty() || track.title.empty()) {
        label_track_artist->set_text(U"");
        label_track_title->set_text(std::filesystem::path{track.path}.filename().string());
      } else {
        label_track_artist->set_text(track.artist);
        label_track_title->set_text(track.title);
      }
      label_track_number->set_text(std::to_string(track_number));
      i32 length_s = track.length_seconds;
      i32 length_m = length_s / 60;
      length_s %= 60;
      std::stringstream ss;
      ss << std::right << std::setfill('0') << std::setw(0) << length_m << ":" << std::setw(2) << length_s;
      label_track_length->set_text(ss.str());

      love_icon->set_is_drawn(db::playlist_loved_tracks().has_track_id(track_id_));
    }

    void set_highlighted(bool new_state) {
      if (highlighted_old == new_state) { return; }
      highlighted_old = new_state;

      std::string txt;
      glm::vec3 text_color;
      if (new_state) {
        panel_right_side->set_texture("track_bg_playing", false);
        txt = "track_bg_playing";
        text_color = glm::vec3{0.55, 0.35, 0.45};
      } else {
        panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
        txt = even ? "track_bg2" : "track_bg1";
        text_color = glm::vec3{0.50, 0.40, 0.48};
      }

      set_texture(txt, false);
      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      label_track_number->set_text_color(text_color * 1.2f);
      label_track_artist->set_text_color(text_color * 0.9f);
      label_track_title->set_text_color(text_color * 1.5f);
      label_track_length->set_text_color(text_color * 0.9f);

      auto& track = db::track_by_id(track_id)->get();
      if (track.is_tombstone()) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      }
    }

    void update() override {
      if (!is_mouse_hovering()) { is_hovered = false; }
      hover->set_width(width);
      hover->set_height(height);
      hover->set_is_drawn(is_hovered);
      Sprite::update();
    }

    void handle_event(Input::InputEventMouseMove& ev) override {
      is_hovered = is_mouse_hovering();
      Button::handle_event(ev);
    }

  public:
    bool is_hovered = false;
    size_t collection_id{};
    size_t album_id{};
    size_t track_id{};

  protected:
    Label* label_track_number{};
    Label* label_track_artist{};
    Label* label_track_title{};
    Panel* panel_right_side{};
    Label* label_track_length{};
    Sprite* hover{};
    Sprite* love_icon{};
    bool even = false;
    bool highlighted_old = false;

    std::optional<size_t> playlist_track{};
    bool pressed = false;
};
