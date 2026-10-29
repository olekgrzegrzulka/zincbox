#pragma once
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "ui/zincgui/button.hpp"
#include "ui/zincgui/default_settings.hpp"

namespace zincgui {

  namespace Input {
    struct InputEventMouseScroll;
    struct InputEventMouseButton;
    struct InputEventKey;
  }; // namespace Input

  class Label;
  class Sprite;
  class Widget;

  class ComboBoxItem : public Button {
    public:
      ComboBoxItem(Root& ui_);
  };

  class ComboBox : public Button {
      using Button::event;

    public:
      ComboBox(Root& ui_);

      void update() override;
      void event(Input::InputEventMouseScroll& ev) override;
      void event(Input::InputEventMouseButton& ev) override;
      void event(Input::InputEventKey& ev) override;

      ComboBox& add_item(std::string_view id, std::string_view label_);

      [[nodiscard]] std::string selected_item_id() const;
      [[nodiscard]] std::string selected_item_label() const;
      [[nodiscard]] i32 selected_index() const;
      ComboBox& select_item_by_label(std::string_view label_);
      ComboBox& select_item_by_id(std::string_view id);
      ComboBox& select_item_by_index(i32 index);

      [[nodiscard]] i32 item_height() const noexcept { return m_item_height; }
      ComboBox& item_height(i32 v) {
        if (m_item_height != v) {
          m_item_height = v;
          mark_dirty();
        }
        return *this;
      }

      ComboBox& on_item_selected(std::function<void()> lambda_select_) {
        m_on_item_selected = std::move(lambda_select_);
        return *this;
      }

      [[nodiscard]] Sprite& icon() noexcept { return m_icon; }
      [[nodiscard]] const Sprite& icon() const noexcept { return m_icon; }
      [[nodiscard]] Sprite& dropdown_bg() noexcept { return m_dropdown_bg; }
      [[nodiscard]] const Sprite& dropdown_bg() const noexcept { return m_dropdown_bg; }
      [[nodiscard]] Sprite& sprite_focus() noexcept { return m_sprite_focus; }
      [[nodiscard]] const Sprite& sprite_focus() const noexcept { return m_sprite_focus; }

    protected:
      void on_button_pressed();
      void on_button_depressed();
      void on_item_pressed(i32 i);

    private:
      Sprite& m_icon;
      Sprite& m_dropdown_bg;
      Sprite& m_sprite_focus;
      i32 m_selected_index = -1;

      std::function<void()> m_on_item_selected = nullptr;

      i32 m_item_height = default_settings::combobox_item_height;
      i32 m_dropdown_padding = default_settings::combobox_dropdown_padding;
      static constexpr i32 m_dropdown_max_length = 10;

      enum class DropDownState : u8 { HIDDEN, APPEARING, DISAPPEARING, VISIBLE };
      DropDownState m_dropdown_state = DropDownState::HIDDEN;
      float m_dropdown_animation_progress = 0.0f;
      float m_target_scroll_progress = 0.0f;
      float m_scroll_progress = 0.0f; // 1.0 per one item scrolled

      std::array<ComboBoxItem*, m_dropdown_max_length + 1>
        m_item_widgets{}; // +1 to accomodate for partially visible extra item
      std::vector<std::pair<std::string, std::string>> m_items;
  };

} // namespace zincgui
