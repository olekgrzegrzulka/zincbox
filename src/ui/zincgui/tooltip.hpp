#pragma once
#include <string>
#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/sprite.hpp"

namespace zincgui {

  class Root;
  class Label;

  enum class ToolTipPosition : u8 { LEFT, RIGHT, ABOVE, BELOW, MANUAL };

  class ToolTip : public Sprite {
    public:
      ToolTip(Root& ui_, std::string_view name_, ToolTipPosition pos_ = ToolTipPosition::RIGHT,
              i32 distance_ = default_settings::tooltip_distance);

      void update() override;

      void set_size_and_position();

      ToolTip& text(const std::string& s);
      ToolTip& clamp(bool value);
      [[nodiscard]] bool clamp() const noexcept { return m_clamp; }

      using Sprite::pos;
      using Sprite::position;

      [[nodiscard]] ToolTipPosition tooltip_pos() const noexcept { return m_pos; }
      [[nodiscard]] ToolTipPosition pos() const noexcept { return m_pos; }
      ToolTip& pos(ToolTipPosition p) {
        m_pos = p;
        set_size_and_position();
        return *this;
      }

      [[nodiscard]] i32 distance() const noexcept { return m_distance; }
      ToolTip& distance(i32 d) {
        m_distance = d;
        set_size_and_position();
        return *this;
      }

      [[nodiscard]] Label* label() noexcept { return m_label; }
      [[nodiscard]] const Label* label() const noexcept { return m_label; }

    private:
      Label* m_label{};
      ToolTipPosition m_pos{};
      i32 m_distance{};
      bool m_clamp = default_settings::tooltip_clamp;
  };

} // namespace zincgui
