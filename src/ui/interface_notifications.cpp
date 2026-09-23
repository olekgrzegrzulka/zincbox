#include <string>
#include <string_view>
#include <vector>
#include "interface_notifications.hpp"
#include "theme_config.hpp"
#include "ui/theme.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/ui.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

constexpr auto OFFSCREEN_Y_POSITION = 120.0f;
constexpr auto FRAMES_PER_SECOND = 60;
constexpr auto NOTIFICATION_DURATION_SECONDS = 4.0f;
constexpr auto INITIAL_TIMER = FRAMES_PER_SECOND * NOTIFICATION_DURATION_SECONDS;
constexpr auto NOTIFICATIONS_Y_OFFSET = -30;
constexpr auto HIDE_TIMER_THRESHOLD = 20;
constexpr auto LERP_FACTOR = 0.75f;

Notification::Notification(zincgui::Root& ui_) : Sprite(ui_, "notification") {
  set_nine_slice_margin(theme::config().notification.nine_slice_margin);
  label = &add_child<Label>();
  set_parent_anchor(Anchor::CENTER);
  set_anchor(Anchor::CENTER);
  label->set_parent_anchor(Anchor::CENTER);
  label->set_anchor(Anchor::CENTER);
  label->set_label_anchor(Anchor::CENTER);

  y_lerped = OFFSCREEN_Y_POSITION;
  timer = INITIAL_TIMER;
}

void Notification::update() {
  if (timer > 0) { timer -= 1; }
  set_size(label->get_text_extents() + vec2f{32.0f, 24.0f});
  Sprite::update();
}

InterfaceNotifications::InterfaceNotifications(zincgui::Root& ui_) : Widget(ui_) {
  static const float scale = zincbox::ui_scale();

  set_parent_anchor(Anchor::BOTTOM_CENTER);
  set_y(NOTIFICATIONS_Y_OFFSET * scale);
  set_anchor(Anchor::BOTTOM_CENTER);
}

void InterfaceNotifications::push(std::string_view message) {
  auto& w = add_child<Notification>();
  w.label->set_text(message);
  w.label->update();
  notifications.emplace_back(&w);
}

void InterfaceNotifications::update() {
  static const float scale = zincbox::ui_scale();

  i32 offset = 0;
  for (auto* w : notifications) {
    float target_y =
      w->timer > HIDE_TIMER_THRESHOLD ? -(theme::config().notification.height + offset) : OFFSCREEN_Y_POSITION;
    w->y_lerped = w->y_lerped * LERP_FACTOR + target_y * (1.0f - LERP_FACTOR);
    w->set_y(w->y_lerped);
    if (w->timer > HIDE_TIMER_THRESHOLD) { offset += w->get_height() + theme::config().notification.spacing * scale; }

    if (w->timer <= 0) { w->set_marked_for_deletion(true); }
  }

  notifications.erase(
    std::remove_if(notifications.begin(), notifications.end(), [](Notification* w) { return w->timer <= 0; }),
    notifications.end());

  Widget::update();
}
