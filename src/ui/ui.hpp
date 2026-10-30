#pragma once
#include <string>
#include <vector>
#include "common/types.hpp"
#include "ui/zincgui/tooltip_controller.hpp"

namespace zincbox::ui {
  void init();
  void deinit();
  [[nodiscard]] bool update(vec2i window_size);
  void draw();
  bool get_mini_player();
  std::string get_selected_tab();
  std::vector<std::string> get_tabs_order();
  i32 get_tracks_scroll_offset();
  i32 get_playlists_scroll_offset();

  void set_mini_player(bool);
  void set_selected_tab(const std::string&);
  void set_tabs_order(std::vector<std::string>);
  void set_tracks_scroll_offset(i32);
  void set_playlists_scroll_offset(i32);

  enum class DecorationHover : u8 {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP,
    LEFT,
    RIGHT,
    BOTTOM,
    INSIDE,
    TITLEBAR,
  };

  TooltipController* get_tooltip_controller();
  DecorationHover get_decoration_hover(i32 mouse_x, i32 mouse_y);
} // namespace zincbox::ui
