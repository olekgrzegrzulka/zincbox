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

      void set_text(const std::string& s);
      void set_clamp(bool value);
      void set_size_and_position();

    protected:
      Label* label{};
      ToolTipPosition pos{};
      i32 distance{};
      bool clamp = default_settings::tooltip_clamp;
  };

} // namespace zincgui
