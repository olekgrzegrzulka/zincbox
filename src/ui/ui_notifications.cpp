#include "ui/ui_notifications.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include "ui/theme.hpp"
#include "ui/theme_config.hpp"
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"
#include "zincbox.hpp"

using namespace zincgui;

constexpr auto OFFSCREEN_Y_POSITION = 120.0f;
constexpr auto FRAMES_PER_SECOND = 60;
constexpr auto NOTIFICATION_DURATION_SECONDS = 4.0f;
constexpr auto INITIAL_TIMER = FRAMES_PER_SECOND * NOTIFICATION_DURATION_SECONDS;
constexpr auto ERROR_DURATION_SECONDS = 8.0f;
constexpr auto INITIAL_TIMER_ERROR = FRAMES_PER_SECOND * ERROR_DURATION_SECONDS;
constexpr auto NOTIFICATIONS_Y_OFFSET = -30;
constexpr auto HIDE_TIMER_THRESHOLD = 20;
constexpr auto LERP_FACTOR = 0.75f;

Notification::Notification(zincgui::Root& ui_, bool error) : Button(ui_) {
  std::string texture_id = error ? "notification_error" : "notification";
  texture_disabled(texture_id)
    .texture_hovered(texture_id)
    .texture_idle(texture_id)
    .texture_pressed(texture_id)
    .texture(texture_id, false);

  on_press([this]() -> void {
    m_timer = HIDE_TIMER_THRESHOLD;
    m_dismissed = true;
  });

  nine_slice_margin(theme::config().notification.nine_slice_margin);
  m_label = &add_child<Label>();
  parent_anchor(Anchor::CENTER)
    .anchor(Anchor::CENTER);
  m_label->parent_anchor(Anchor::CENTER)
    .anchor(Anchor::CENTER)
    .label_anchor(Anchor::CENTER);

  m_y_lerped = OFFSCREEN_Y_POSITION;
  m_timer = error ? INITIAL_TIMER_ERROR : INITIAL_TIMER;
}

void Notification::update() {
  if (m_timer > 0) { m_timer -= 1; }
  size(m_label->text_extents() + vec2i{32, 24});
  Button::update();
}

InterfaceNotifications::InterfaceNotifications(zincgui::Root& ui_) : Widget(ui_) {
  const float SCALE = zincbox::ui_scale();

  parent_anchor(Anchor::BOTTOM_CENTER)
    .y(NOTIFICATIONS_Y_OFFSET * SCALE)
    .anchor(Anchor::BOTTOM_CENTER);
}

void InterfaceNotifications::update() {
  const float SCALE = zincbox::ui_scale();

  i32 offset = 0;
  for (auto* w : m_notifications) {
    bool visible = w->timer() > HIDE_TIMER_THRESHOLD || w->timer() < 0;
    float target_y = visible ? -(theme::config().notification.height + offset) : OFFSCREEN_Y_POSITION;
    w->y_lerped((w->y_lerped() * LERP_FACTOR) + (target_y * (1.0f - LERP_FACTOR)));
    w->y(w->y_lerped());
    if (visible) { offset += w->height() + (theme::config().notification.spacing * SCALE); }

    if (w->timer() == 0) { w->marked_for_deletion(true); }
  }

  m_notifications.erase(std::remove_if(m_notifications.begin(), m_notifications.end(),
                                       [](Notification* w) -> bool { return w->timer() == 0; }),
                        m_notifications.end());

  Widget::update();
}

void InterfaceNotifications::push(std::string_view message) {
  auto& w = add_child<Notification>();
  w.label()->text(message);
  m_notifications.emplace_back(&w);
}

void InterfaceNotifications::push_error(std::string_view message) {
  auto& w = add_child<Notification>(true);
  w.label()->text(message);
  m_notifications.emplace_back(&w);
}

Notification* InterfaceNotifications::push_persistent(std::string_view message) {
  auto& w = add_child<Notification>();
  w.label()->text(message);
  w.timer(-1);
  m_notifications.emplace_back(&w);
  return &w;
}

void InterfaceNotifications::dismiss(Notification* w) { dismiss_after(w, 0); }

void InterfaceNotifications::dismiss_after(Notification* w, i32 frames) {
  if (w && !w->dismissed()) {
    w->timer(frames + HIDE_TIMER_THRESHOLD)
      .dismissed(true);
  }
}
