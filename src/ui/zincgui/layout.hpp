#pragma once
#include <string_view>
#include "ui/zincgui/widget.hpp"

namespace zincgui::layout_engine {

  Layout parse_layout_string(std::string_view def, const Layout& fallback);
  void apply_layout(Widget& container);

} // namespace zincgui::layout_engine
