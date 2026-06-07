#pragma once
#include <string>
#include "ui/theme.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

class Notification : public Sprite {
  public:
    Notification(UI& ui_) : Sprite(ui_, "notification") {
      set_nine_slice_margin(6);
      label = &add_child<Label>();
      set_parent_anchor(Anchor::CENTER);
      set_anchor(Anchor::CENTER);
      label->set_parent_anchor(Anchor::CENTER);
      label->set_anchor(Anchor::CENTER);
      label->set_label_anchor(Anchor::CENTER);
    }

    void update() override {
      if (timer > 0) {
        timer -= 1;
      } else {
        set_marked_for_deletion(true);
      }
      set_size(label->get_text_extents() + vec2f{32, 24});
      Sprite::update();
    }

    i32 timer = 60 * 3.5;
    Label* label{};
};

class InterfaceNotifications : public Widget {
  public:
    InterfaceNotifications(UI& ui_) : Widget(ui_) {
      set_is_drawn_on_top(true);
      set_parent_anchor(Anchor::BOTTOM_CENTER);
      set_y(-30);
      set_anchor(Anchor::BOTTOM_CENTER);
    }

    void push(std::u32string_view message) {
      auto& w = add_child<Notification>();
      w.label->set_text(message);
      w.label->update();
    }

    void update() override {
      i32 offset = 0;
      for (auto& w : get_children()) {
        w->set_y(-(theme::get_prop("notification_height").as_i32(10) + offset));
        offset += w->get_height() + theme::get_prop("notification_spacing").as_i32(10);
      }
      Widget::update();
    }
};
