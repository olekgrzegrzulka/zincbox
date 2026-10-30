#pragma once

#include <string_view>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/tooltip.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {
  class Root;
} // namespace zincgui

class TooltipController : public zincgui::Widget {
  public:
    TooltipController(const TooltipController&) = delete;
    TooltipController(TooltipController&&) = delete;
    TooltipController& operator=(const TooltipController&) = delete;
    TooltipController& operator=(TooltipController&&) = delete;
    explicit TooltipController(zincgui::Root& ui_);
    ~TooltipController() override = default;

    void update() override;

    TooltipController& show(const zincgui::TooltipDescriptor& d);

    TooltipController& show(vec2i at, std::string_view text,
                            zincgui::ToolTipPosition pos = zincgui::ToolTipPosition::BELOW,
                            i32 distance = zincgui::default_settings::tooltip_distance, bool show_arrow = true,
                            bool clamp = zincgui::default_settings::tooltip_clamp);

    TooltipController& show(const zincgui::Widget& target, std::string_view text,
                            zincgui::ToolTipPosition pos = zincgui::ToolTipPosition::BELOW,
                            i32 distance = zincgui::default_settings::tooltip_distance, bool show_arrow = true,
                            bool clamp = zincgui::default_settings::tooltip_clamp);

    TooltipController& hide();

    [[nodiscard]] zincgui::ToolTip* tooltip() noexcept { return m_tooltip; }
    [[nodiscard]] const zincgui::ToolTip* tooltip() const noexcept { return m_tooltip; }

    [[nodiscard]] bool enabled() const noexcept { return m_enabled; }
    template <typename Self> Self& enabled(this Self& self, bool state) {
      self.m_enabled = state;
      if (!state) { self.hide(); }
      return self;
    }

  private:
    zincgui::ToolTip* m_tooltip{};
    bool m_requested_this_frame = false;
    bool m_enabled = true;
};
