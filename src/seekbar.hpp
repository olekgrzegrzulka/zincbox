#pragma once
#include "ui/sprite.hpp"
#include "ui/widget.hpp"

class UI;

class SeekBar : public Widget {
public:
  SeekBar(UI& ui_) : Widget(ui_), bg(add_child<Sprite>("seekbar_bg")), progress_bar(add_child<Sprite>("seekbar_progress")), thumb(add_child<Sprite>("seekbar_thumb")) {
    bg.set_parent_anchor(Anchor::LEFT);
    bg.set_anchor(Anchor::LEFT);
    bg.set_texture("seekbar_bg", false);
    bg.set_nine_slice_margin(6.0f);

    progress_bar.set_parent_anchor(Anchor::LEFT);
    progress_bar.set_anchor(Anchor::LEFT);
    progress_bar.set_nine_slice_margin(6.0f);

    thumb.set_parent_anchor(Anchor::LEFT);
    thumb.set_anchor(Anchor::CENTER);
    thumb.set_nine_slice_margin(6.0f);
    thumb.set_size(14, 14);
  }

  void update() override {
    bg.set_width(width);
    bg.set_height(12);
    progress_bar.set_width(width);
    progress_bar.set_height(12);

    static i32 t = 0;
    t += 1;
    progress_bar.set_width(((std::sin(t * 0.005) * 0.8 + std::sin(t * 0.01178) * 0.2) + 1.0) * 0.5 * width);
    thumb.set_x(progress_bar.get_width());
    Widget::update();
  }

protected:
  Sprite& bg;
  Sprite& progress_bar;
  Sprite& thumb;
};
