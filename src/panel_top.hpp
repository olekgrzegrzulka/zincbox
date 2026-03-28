#pragma once
#include <functional>
#include <optional>
#include "tab_bar.hpp"
#include "types.hpp"
#include "ui/panel.hpp"

class PanelTop : public Panel {
  public:
    PanelTop(UI& ui_);
    void update() override;
    void recreate(std::optional<size_t> selected_collection_id);

  public:
    std::function<void(size_t collection_id)> on_collection_opened{};
    std::function<void()> on_queue_view_opened{};
    std::function<void(size_t collection_id, vec2i at)> on_show_collection_actions_popover{};

  protected:
    TabBar& tab_bar;
};
