#pragma once
#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;
  class Label;

  enum class ToolTipPosition : u8 { ABOVE, BELOW };

  struct TooltipDescriptor {
      std::string_view text;
      vec2i at{};
      ToolTipPosition pos = ToolTipPosition::BELOW;
      i32 distance = default_settings::tooltip_distance;
      bool show_arrow = true;
      bool clamp = default_settings::tooltip_clamp;
  };

  class ToolTip : public Sprite {
    public:
      ToolTip(const ToolTip&) = delete;
      ToolTip(ToolTip&&) = delete;
      ToolTip& operator=(const ToolTip&) = delete;
      ToolTip& operator=(ToolTip&&) = delete;
      explicit ToolTip(Root& ui_);
      ~ToolTip() override = default;

      void update() override;

      void apply(const TooltipDescriptor& d);
      void update_placement();

      ToolTip& text(std::string_view s);

      [[nodiscard]] Label* label() noexcept { return m_label; }
      [[nodiscard]] const Label* label() const noexcept { return m_label; }

      [[nodiscard]] Sprite* arrow() noexcept { return m_arrow; }
      [[nodiscard]] const Sprite* arrow() const noexcept { return m_arrow; }

      [[nodiscard]] ToolTipPosition tooltip_pos() const noexcept { return m_pos; }
      template <typename Self> Self& tooltip_pos(this Self& self, ToolTipPosition p) {
        self.m_pos = p;
        return self;
      }

      [[nodiscard]] vec2i target_pos() const noexcept { return m_target_pos; }
      template <typename Self> Self& target_pos(this Self& self, vec2i p) {
        self.m_target_pos = p;
        return self;
      }

      [[nodiscard]] i32 distance() const noexcept { return m_distance; }
      template <typename Self> Self& distance(this Self& self, i32 d) {
        self.m_distance = d;
        return self;
      }

      [[nodiscard]] i32 arrow_offset() const noexcept { return m_arrow_offset; }
      template <typename Self> Self& arrow_offset(this Self& self, i32 offset) {
        self.m_arrow_offset = offset;
        return self;
      }

      [[nodiscard]] bool show_arrow() const noexcept { return m_show_arrow; }
      template <typename Self> Self& show_arrow(this Self& self, bool value) {
        self.m_show_arrow = value;
        return self;
      }

      [[nodiscard]] bool clamp() const noexcept { return m_clamp; }
      template <typename Self> Self& clamp(this Self& self, bool value) {
        self.m_clamp = value;
        return self;
      }

    private:
      Label* m_label{};
      Sprite* m_arrow{};
      ToolTipPosition m_pos = ToolTipPosition::BELOW;
      vec2i m_target_pos{};
      i32 m_distance = default_settings::tooltip_distance;
      i32 m_arrow_offset = 0;
      bool m_show_arrow = true;
      bool m_clamp = default_settings::tooltip_clamp;
  };

} // namespace zincgui
