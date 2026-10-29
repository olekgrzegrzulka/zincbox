#pragma once
#include <functional>
#include <string_view>
#include <utility>
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/color_rect.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/label.hpp"
#include "ui/zincgui/root.hpp"
#include "ui/zincgui/texture_atlas.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Checkbox : public Widget {
    public:
      Checkbox(Root& ui_) : Widget(ui_) { init(""); }

      Checkbox(Root& ui_, std::string_view label_text) : Widget(ui_) { init(label_text); }

      void update() override {
        m_button->size(width(), height());
        // m_inner->size(width(), height());

        m_sprite_check->is_drawn(m_checked);

        switch (m_button->state()) {
        case ButtonState::IDLE:
          m_sprite_checkbox->uv_start(m_uv_checkbox_idle.start).uv_end(m_uv_checkbox_idle.end);
          break;
        case ButtonState::HOVERED:
          m_sprite_checkbox->uv_start(m_uv_checkbox_hovered.start).uv_end(m_uv_checkbox_hovered.end);
          break;
        case ButtonState::PRESSED:
          m_sprite_checkbox->uv_start(m_uv_checkbox_pressed.start).uv_end(m_uv_checkbox_pressed.end);
          break;
        case ButtonState::DISABLED:
          m_sprite_checkbox->uv_start(m_uv_checkbox_disabled.start).uv_end(m_uv_checkbox_disabled.end);
          break;
        }

        Widget::update();
      }

      void init(std::string_view label_text) {
        is_focusable(true).size(default_settings::checkbox_size);
        if (default_settings::checkbox_fixed_height) { fixed_height(height()); }
        hit_test_behavior(zincgui::HitTestBehavior::PASS_THROUGH);

        m_uv_checkbox_idle = ui().texture_atlas().get(default_settings::checkbox_texture_idle)->get();
        m_uv_checkbox_hovered = ui().texture_atlas().get(default_settings::checkbox_texture_hovered)->get();
        m_uv_checkbox_pressed = ui().texture_atlas().get(default_settings::checkbox_texture_pressed)->get();
        m_uv_checkbox_disabled = ui().texture_atlas().get(default_settings::checkbox_texture_disabled)->get();
        m_uv_checkbox_check = ui().texture_atlas().get(default_settings::checkbox_texture_check)->get();

        clip_children(true);

        m_button = &add_child<Button>();
        m_button->ignore_parents_layout(true).is_self_drawn(false).switch_mode(true);
        m_button->on_press([this]() -> void { checked(true); });
        m_button->on_depress([this]() -> void { checked(false); });

        m_inner = &add_child<Widget>();
        m_inner->ignore_parents_layout(true)
          .layout(default_settings::checkbox_layout)
          .anchor(Anchor::CENTER)
          .parent_anchor(Anchor::CENTER);

        m_sprite_checkbox = &m_inner->add_child<Sprite>(default_settings::checkbox_texture_idle);
        m_sprite_checkbox->fixed_width(m_sprite_checkbox->width()).fixed_height(m_sprite_checkbox->height());
        m_sprite_check = &m_sprite_checkbox->add_child<Sprite>(default_settings::checkbox_texture_check);
        m_sprite_check->parent_anchor(Anchor::CENTER).anchor(Anchor::CENTER);

        m_label = &m_inner->add_child<Label>(label_text);
        m_label->label_anchor(Anchor::LEFT);
      }

      [[nodiscard]] bool checked() const noexcept { return m_button ? m_button->is_switched() : m_checked; }
      Checkbox& checked(bool value) {
        if (m_checked == value) { return *this; }
        m_checked = value;
        m_button->is_switched(m_checked);
        m_sprite_check->is_drawn(m_checked);
        if (m_on_value_changed) { m_on_value_changed(); }
        return *this;
      }

      Checkbox& checkbox_anchor(Anchor anchor) {
        m_inner->anchor(anchor).parent_anchor(anchor);
        return *this;
      }

      Checkbox& on_value_changed(std::function<void()> lambda) {
        m_on_value_changed = std::move(lambda);
        return *this;
      }

      [[nodiscard]] Button* button() noexcept { return m_button; }
      [[nodiscard]] const Button* button() const noexcept { return m_button; }
      [[nodiscard]] Sprite* sprite_checkbox() noexcept { return m_sprite_checkbox; }
      [[nodiscard]] const Sprite* sprite_checkbox() const noexcept { return m_sprite_checkbox; }
      [[nodiscard]] Sprite* sprite_check() noexcept { return m_sprite_check; }
      [[nodiscard]] const Sprite* sprite_check() const noexcept { return m_sprite_check; }
      [[nodiscard]] Label* label() noexcept { return m_label; }
      [[nodiscard]] const Label* label() const noexcept { return m_label; }
      [[nodiscard]] Widget* inner() noexcept { return m_inner; }
      [[nodiscard]] const Widget* inner() const noexcept { return m_inner; }

    private:
      bool m_checked = false;
      std::function<void()> m_on_value_changed;
      Button* m_button{};
      Sprite* m_sprite_checkbox{};
      Sprite* m_sprite_check{};
      Label* m_label{};
      Widget* m_inner{};
      TextureAtlasData m_uv_checkbox_idle{};
      TextureAtlasData m_uv_checkbox_hovered{};
      TextureAtlasData m_uv_checkbox_pressed{};
      TextureAtlasData m_uv_checkbox_disabled{};
      TextureAtlasData m_uv_checkbox_check{};
  };

} // namespace zincgui
