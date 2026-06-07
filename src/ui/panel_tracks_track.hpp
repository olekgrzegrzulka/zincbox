#pragma once
#include <sstream>
#include <string>
#include "common/types.hpp"
#include "core/musicdb/musicdb.hpp"
#include "theme.hpp"
#include "ui_generic/button.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"

class WidgetTrack : public Button {
  public:
    WidgetTrack(UI& ui_, size_t track_id_, size_t track_number, bool even_) : Button(ui_) {
      setup(track_id_, track_number, even_);
    }

    void setup(size_t track_id_, size_t track_number, bool even_) {
      track_id = track_id_;
      even = even_;
      const i32 track_height = theme::get_prop("tracklist_track_height").as_i32();
      const auto& track = db::track_by_id(track_id)->get();
      const std::string txt = even ? "track_bg2" : "track_bg1";

      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      set_texture(txt, false);
      set_nine_slice_margin(8.0f);
      set_height(track_height);

      if (!label_track_number) {
        label_track_number = &add_child<Label>();
        label_track_number->set_resize_to_text_extents(false);
        label_track_number->set_label_anchor(Anchor::LEFT);
        label_track_number->set_size(20, track_height);
        label_track_number->set_text_color(theme::get_prop("tracklist_track_number_text_color").as_rgba());
        label_track_number->set_x(5);
      }
      label_track_number->set_text(std::to_string(track_number));

      if (!label_track_artist) {
        label_track_artist = &add_child<Label>();
        label_track_artist->set_label_anchor(Anchor::LEFT);
        label_track_artist->set_height(track_height);
        label_track_artist->set_text_color(theme::get_prop("tracklist_artist_text_color").as_rgba());
        label_track_artist->set_resize_to_text_extents(false);
      }
      label_track_artist->set_x(label_track_number->get_x() + label_track_number->get_width() + 5);
      if (track.artist.empty() || track.title.empty()) {
        label_track_artist->set_text("");
        label_track_artist->set_width(0);
      } else {
        label_track_artist->set_text(track.artist);
        label_track_artist->set_width(label_track_artist->get_text_extents().x);

        // hack to get text extents to update :(
        label_track_artist->mark_dirty();
        label_track_artist->update();
      }

      if (!label_track_title) {
        label_track_title = &add_child<Label>();
        label_track_title->set_resize_to_text_extents(false);
        label_track_title->set_label_anchor(Anchor::LEFT);
        label_track_title->set_height(track_height);
        label_track_title->set_text_color(theme::get_prop("tracklist_title_text_color").as_rgba());
      }
      label_track_title->set_x(label_track_artist->get_x() + label_track_artist->get_text_extents().x + 5);
      if (track.artist.empty() || track.title.empty()) {
        label_track_title->set_text(std::filesystem::path{track.path}.filename().string());
      } else {
        label_track_title->set_text(track.title);
      }

      if (!panel_right_side) {
        panel_right_side = &add_child<Sprite>("panel_tracks");
        panel_right_side->set_height(track_height);
        panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
        panel_right_side->set_layout("m:8 s:8 rtl fit");
        panel_right_side->set_anchor(Anchor::RIGHT);
        panel_right_side->set_parent_anchor(Anchor::RIGHT);
        panel_right_side->set_ignore_parents_layout(true);
      }

      if (!label_track_length) {
        label_track_length = &panel_right_side->add_child<Label>();
        label_track_length->set_height(track_height);
        label_track_length->set_text_color(theme::get_prop("tracklist_length_text_color").as_rgba());
        label_track_length->set_resize_to_text_extents(false);
      }

      label_track_length->set_text(track.pretty_length());
      // hack to get text extents to update :(
      label_track_length->mark_dirty();
      label_track_length->update();
      label_track_length->set_width(label_track_length->get_text_extents().x);

      if (!love_icon) {
        love_icon = &panel_right_side->add_child<Sprite>("love");
        love_icon->set_size(12, 12);
        love_icon->set_nine_slice_margin(0.0f);
      }
      love_icon->set_is_drawn(db::playlist_loved_tracks().has_track_id(track_id_));

      if (track.is_tombstone()) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      }

      if (!hover) { hover = &add_child<Sprite>("track_hovered"); }

      set_playback_error(track.is_playback_error());
      update_text_colors();
    }

    void set_highlighted(bool highlighted_new) {
      if (highlighted == highlighted_new) { return; }
      highlighted = highlighted_new;

      update_text_colors();
    }

    void set_playback_error(bool playback_error_new) {
      if (playback_error == playback_error_new) { return; }
      playback_error = playback_error_new;

      update_text_colors();
    }

    void update_text_colors() {
      std::string txt;
      if (highlighted) {
        panel_right_side->set_texture("track_bg_playing", false);
        txt = "track_bg_playing";
      } else {
        panel_right_side->set_texture(even ? "track_bg2" : "track_bg1", false);
        txt = even ? "track_bg2" : "track_bg1";
      }

      set_texture(txt, false);
      set_texture_idle(txt);
      set_texture_hovered(txt);
      set_texture_disabled(txt);
      set_texture_pressed(txt);
      label_track_number->set_text_color(theme::get_prop("tracklist_track_number_text_color").as_rgba());
      label_track_artist->set_text_color(theme::get_prop("tracklist_artist_text_color").as_rgba());
      label_track_title->set_text_color(theme::get_prop("tracklist_title_text_color").as_rgba());
      label_track_length->set_text_color(theme::get_prop("tracklist_length_text_color").as_rgba());

      auto& track = db::track_by_id(track_id)->get();
      if (track.is_tombstone() || playback_error) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 0.6f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 0.6f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 0.6f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 0.6f);
      } else if (highlighted) {
        label_track_number->set_text_color(label_track_number->get_text_color() * 1.5f);
        label_track_artist->set_text_color(label_track_artist->get_text_color() * 1.5f);
        label_track_title->set_text_color(label_track_title->get_text_color() * 1.5f);
        label_track_length->set_text_color(label_track_length->get_text_color() * 1.5f);
      }
    }

    void update() override {
      if (!is_mouse_hovering()) { is_hovered = false; }
      hover->set_width(width);
      hover->set_height(height);
      hover->set_is_drawn(is_hovered);
      Sprite::update();
    }

    void event(Input::InputEventMouseMove& ev) override {
      is_hovered = is_mouse_hovering();
      Button::event(ev);
    }

  public:
    bool is_hovered = false;
    size_t track_id{};

  protected:
    Label* label_track_number{};
    Label* label_track_artist{};
    Label* label_track_title{};
    Sprite* panel_right_side{};
    Label* label_track_length{};
    Sprite* hover{};
    Sprite* love_icon{};
    bool even = false;
    bool highlighted = false;
    bool playback_error = false;

    std::optional<size_t> playlist_track{};
    bool pressed = false;
};
