#pragma once
#include <string>
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

    TooltipController& show_at(vec2i screen_pos, std::string_view text,
                               zincgui::Anchor anchor = zincgui::Anchor::TOP_LEFT,
                               bool clamp_to_window = zincgui::default_settings::tooltip_clamp);

    TooltipController& show_for_widget(const zincgui::Widget& target, std::string_view text,
                                       zincgui::ToolTipPosition pos = zincgui::ToolTipPosition::BELOW,
                                       i32 distance = zincgui::default_settings::tooltip_distance,
                                       i32 cross_axis_offset = 0,
                                       bool clamp_to_window = zincgui::default_settings::tooltip_clamp);

    TooltipController& hide();

    [[nodiscard]] bool enabled() const noexcept { return m_enabled; }
    template <typename Self> Self& enabled(this Self& self, bool state) {
      self.m_enabled = state;
      if (!state) { self.hide(); }
      return self;
    }

  private:
    void apply_placement(vec2i screen_pos, std::string_view text, zincgui::Anchor anchor, bool clamp_to_window);

    zincgui::ToolTip* m_tooltip{};
    std::string m_current_text;
    vec2i m_target_pos{};
    zincgui::Anchor m_target_anchor = zincgui::Anchor::TOP_LEFT;
    bool m_clamp = zincgui::default_settings::tooltip_clamp;
    bool m_requested_this_frame = false;
    bool m_enabled = true;
};
