#pragma once
#include <string>
#include <vector>
#include "common/types.hpp"

namespace interface {
  void init();
  void deinit();
  void update(vec2i window_size);
  bool get_mini_player();
  std::u32string get_selected_tab();
  std::vector<std::u32string> get_tabs_order();
  i32 get_tracks_scroll_offset();
  i32 get_playlists_scroll_offset();

  void set_mini_player(bool);
  void set_selected_tab(std::u32string);
  void set_tabs_order(std::vector<std::u32string>);
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

  DecorationHover get_decoration_hover();
} // namespace interface
