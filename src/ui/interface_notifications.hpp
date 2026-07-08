#pragma once
#include <string>
#include "ui/theme.hpp"
#include "ui_generic/label.hpp"
#include "ui_generic/sprite.hpp"
#include "ui_generic/ui.hpp"
#include "ui_generic/widget.hpp"

constexpr auto OFFSCREEN_Y_POSITION = 120.0f;
constexpr auto FRAMES_PER_SECOND = 60;
constexpr auto NOTIFICATION_DURATION_SECONDS = 4.0f;
constexpr auto INITIAL_TIMER = FRAMES_PER_SECOND * NOTIFICATION_DURATION_SECONDS;
constexpr auto NOTIFICATIONS_Y_OFFSET = -30;
constexpr auto HIDE_TIMER_THRESHOLD = 20;
constexpr auto LERP_FACTOR = 0.75f;

class Notification : public Sprite {
  public:
    Notification(UI& ui_) : Sprite(ui_, "notification") {
      set_nine_slice_margin(theme::get_prop("notification_nine_slice_margin").as_i32(6));
      label = &add_child<Label>();
      set_parent_anchor(Anchor::CENTER);
      set_anchor(Anchor::CENTER);
      label->set_parent_anchor(Anchor::CENTER);
      label->set_anchor(Anchor::CENTER);
      label->set_label_anchor(Anchor::CENTER);
    }

    void update() override {
      if (timer > 0) { timer -= 1; }
      set_size(label->get_text_extents() + vec2f{32.0f, 24.0f});
      Sprite::update();
    }

    float y_lerped = OFFSCREEN_Y_POSITION;
    i32 timer = INITIAL_TIMER;
    Label* label{};
};

class InterfaceNotifications : public Widget {
  public:
    InterfaceNotifications(UI& ui_) : Widget(ui_) {
      set_is_drawn_on_top(true);
      set_parent_anchor(Anchor::BOTTOM_CENTER);
      set_y(NOTIFICATIONS_Y_OFFSET);
      set_anchor(Anchor::BOTTOM_CENTER);
    }

    void push(std::u32string_view message) {
      auto& w = add_child<Notification>();
      w.label->set_text(message);
      w.label->update();
      notifications.emplace_back(&w);
    }

    void update() override {
      i32 offset = 0;
      for (auto* w : notifications) {
        float target_y = w->timer > HIDE_TIMER_THRESHOLD ? -(theme::get_prop("notification_height").as_i32(10) + offset)
                                                         : OFFSCREEN_Y_POSITION;
        w->y_lerped = w->y_lerped * LERP_FACTOR + target_y * (1.0f - LERP_FACTOR);
        w->set_y(w->y_lerped);
        if (w->timer > HIDE_TIMER_THRESHOLD) {
          offset += w->get_height() + theme::get_prop("notification_spacing").as_i32(10);
        }

        if (w->timer <= 0) { w->set_marked_for_deletion(true); }
      }

      notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(), [](Notification* w) { return w->timer <= 0; }),
        notifications.end());

      Widget::update();
    }

  protected:
    std::vector<Notification*> notifications;
};
