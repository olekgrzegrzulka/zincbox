#pragma once
#include <functional>
#include <string>
#include <string_view>
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
      void draw() override;
      void event(Input::InputEventMouseScroll& ev) override;
      void event(Input::InputEventMouseButton& ev) override;
      void event(Input::InputEventKey& ev) override;

      void on_item_selected(std::function<void()> lambda_select_);
      void add_item(std::string_view id, std::string_view label_);

      std::string get_selected_item_id() const;
      std::string get_selected_item_label() const;
      i32 get_selected_index() const;
      void select_item_by_label(std::string_view label_);
      void select_item_by_id(std::string_view id);
      void select_item_by_index(i32 index);

    protected:
      void on_button_pressed();
      void on_button_depressed();
      void on_item_pressed(i32 i);

    protected:
      Sprite& icon;
      Sprite& dropdown_bg;
      Sprite& sprite_focus;
      i32 selected_index = -1;

      std::function<void()> lambda_select = nullptr;

      i32 item_height = default_settings::combobox_item_height;
      i32 dropdown_padding = default_settings::combobox_dropdown_padding;
      static constexpr i32 dropdown_max_length = 10;

      enum class DropDownState : u8 { HIDDEN, APPEARING, DISAPPEARING, VISIBLE };
      DropDownState dropdown_state = ComboBox::DropDownState::HIDDEN;
      float dropdown_animation_progress = 0.0;
      float target_scroll_progress = 0.0;
      float scroll_progress = 0.0; // 1.0 per one item scrolled

      std::array<ComboBoxItem*, dropdown_max_length + 1>
        item_widgets{}; // +1 to accomodate for partially visible extra item
      std::vector<std::pair<std::string, std::string>> items;

    public:
      WIDGET_DEF_SETTER_DIRTY(item_height);
  };

} // namespace zincgui
