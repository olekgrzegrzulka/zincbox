#pragma once
#include <string>
#include "common/color.hpp"
#include "common/types.hpp"

namespace zincgui::default_settings {

  // Global / Theme / Scaling
  inline i32 default_font_size = 14;
  inline rgba text_color = rgba{255, 255, 255, 255};
  inline rgba text_color_disabled = rgba{128, 128, 128, 255};
  inline rgba text_caret_color = rgba{255, 255, 255, 255};

  // Widget
  inline vec2i widget_size = {64, 64};
  inline float widget_weight = 1.0f;
  inline i32 widget_hover_max_depth = 127;
  inline std::string debug_rect_texture = "red";
  inline float debug_rect_nine_slice_margin = 1.0f;
  inline float debug_rect_nine_slice_scale = 1.0f;

  // Sprite
  inline float sprite_nine_slice_margin = 1.0f;
  inline float sprite_nine_slice_scale = 1.0f;
  inline i32 sprite_default_texture_width = 16;
  inline i32 sprite_default_texture_height = 16;
  inline u32 sprite_default_atlas_index = 1;

  // ColorRect
  inline vec3f color_rect_color = {0.0f, 0.0f, 0.0f};
  inline float color_rect_opacity = 1.0f;

  // Label
  inline float label_line_spacing = 4.0f;
  inline bool label_resize_to_text_extents = true;
  inline bool label_parse_tags = true;
  inline bool label_word_wrap = false;
  inline bool label_ellipsis = false;
  inline float label_bold_extra_advance = 0.5f;
  inline float label_fallback_dot_advance = 4.0f;

  // Button
  inline vec2i button_size = {64, 24};
  inline bool button_fixed_height = true;
  inline float button_nine_slice_margin = 10.0f;
  inline float button_nine_slice_scale = 1.0f;
  inline std::string button_texture_idle = "button_idle";
  inline std::string button_texture_hovered = "button_hovered";
  inline std::string button_texture_pressed = "button_pressed";
  inline std::string button_texture_disabled = "button_disabled";

  // Checkbox
  inline vec2i checkbox_size = {64, 24};
  inline bool checkbox_fixed_height = true;
  inline std::string checkbox_layout = "ltr fill expand fit s:4";
  inline std::string checkbox_texture_idle = "checkbox_idle";
  inline std::string checkbox_texture_hovered = "checkbox_hovered";
  inline std::string checkbox_texture_pressed = "checkbox_pressed";
  inline std::string checkbox_texture_disabled = "checkbox_disabled";
  inline std::string checkbox_texture_check = "checkbox_check";

  // ComboBox, ComboBoxItem
  inline vec2i combobox_size = {64, 24};
  inline bool combobox_fixed_height = true;
  inline float combobox_nine_slice_margin = 10.0f;
  inline float combobox_item_nine_slice_margin = 6.0f;
  inline i32 combobox_item_label_offset_x = 2;
  inline i32 combobox_icon_offset_x = -8;
  inline i32 combobox_item_height = 20;
  inline i32 combobox_dropdown_padding = 1;
  inline float combobox_scroll_snap_threshold = 0.02f;
  inline float combobox_scroll_lerp_weight = 0.4f;
  inline float combobox_dropdown_anim_speed = 0.3f;
  inline std::string combobox_texture_idle = "combobox_idle";
  inline std::string combobox_texture_hovered = "combobox_hovered";
  inline std::string combobox_texture_pressed = "combobox_pressed";
  inline std::string combobox_texture_disabled = "combobox_disabled";
  inline std::string combobox_texture_expand = "combobox_expand";
  inline std::string combobox_texture_contract = "combobox_contract";
  inline std::string combobox_texture_panel = "panel_combobox";
  inline std::string combobox_texture_focus = "combobox_focus";
  inline std::string combobox_item_texture_idle = "combobox_item_idle";
  inline std::string combobox_item_texture_hovered = "combobox_item_hovered";
  inline std::string combobox_item_texture_pressed = "combobox_item_pressed";
  inline std::string combobox_item_texture_disabled = "combobox_item_disabled";

  // Slider
  inline vec2i slider_size = {100, 10};
  inline bool slider_fixed_height = true;
  inline i32 slider_track_thickness = 10;
  inline i32 slider_thumb_length = 10;
  inline i32 slider_thumb_thickness = 10;
  inline i32 slider_drag_area_inflation = 0;
  inline float slider_min_value = -10.0f;
  inline float slider_max_value = 10.0f;
  inline float slider_sensitivity = 40.0f;
  inline float slider_track_nine_slice_margin = 6.0f;
  inline float slider_thumb_nine_slice_margin = 6.0f;
  inline double slider_anim_speed_factor = 0.003;
  inline double slider_anim_lerp_min = 0.6;
  inline double slider_anim_lerp_max = 0.9;
  inline std::string slider_texture_thumb_pressed = "slider_thumb_pressed";
  inline std::string slider_texture_thumb_hovered = "slider_thumb_hovered";
  inline std::string slider_texture_thumb_idle = "slider_thumb_idle";
  inline std::string slider_texture_track_inactive = "slider_track_inactive";
  inline std::string slider_texture_track_active = "slider_track_active";

  // ScrollBar
  inline vec2i scrollbar_size = {100, 10};
  inline bool scrollbar_fixed_width = true;
  inline i32 scrollbar_page_size = 25;
  inline i32 scrollbar_content_size = 100;
  inline float scrollbar_min_thumb_ratio = 0.2f;
  inline float scrollbar_min_thumb_max_px = 40.0f;
  inline std::string scrollbar_texture_track_inactive = "scrollbar_track_inactive";
  inline std::string scrollbar_texture_track_active = "scrollbar_track_active";
  inline std::string scrollbar_texture_thumb_pressed = "scrollbar_thumb_pressed";
  inline std::string scrollbar_texture_thumb_hovered = "scrollbar_thumb_hovered";
  inline std::string scrollbar_texture_thumb_idle = "scrollbar_thumb_idle";
  inline i32 scrollbar_sensitivity = 40;

  // TextInput
  inline vec2i text_input_size = {64, 24};
  inline bool text_input_fixed_height = true;
  inline float text_input_nine_slice_margin = 10.0f;
  inline vec2i text_input_label_offset = {2, 0};
  inline vec2i text_input_caret_size = {1, 14};
  inline i32 text_input_caret_offset_x = 1;
  inline float text_input_caret_blink_time = 250.0f;
  inline float text_input_backspace_echo_initial = 250.0f;
  inline float text_input_backspace_echo = 19.0f;
  inline float text_input_clock_step = 10.0f;
  inline std::string text_input_texture_focused = "text_input_focused";
  inline std::string text_input_texture_idle = "text_input_idle";

  // Spinner
  inline vec2i spinner_size = {64, 24};
  inline bool spinner_fixed_height = true;
  inline i32 spinner_min_value = 0;
  inline i32 spinner_max_value = 100;
  inline i32 spinner_button_step = 1;
  inline float spinner_button_nine_slice_margin = 10.0f;
  inline i32 spinner_button_offset_x = 1;
  inline i32 spinner_label_padding_x = 2;
  inline i32 spinner_button_height_padding = 2;
  inline i32 spinner_postfix_offset_x = 1;
  inline i32 spinner_echo_length_initial = 40;
  inline i32 spinner_echo_length = 2;
  inline std::string spinner_texture_increase = "spinner_increase";
  inline std::string spinner_texture_decrease = "spinner_decrease";
  inline std::string spinner_texture_increase_icon = "spinner_increase_icon";
  inline std::string spinner_texture_decrease_icon = "spinner_decrease_icon";

  // ToolTip
  inline i32 tooltip_distance = 16;
  inline float tooltip_nine_slice_margin = 4.0f;
  inline vec2i tooltip_padding = {20, 20};
  inline bool tooltip_clamp = true;
  inline std::string tooltip_texture = "tooltip";

  // FontFace, TextureAtlas
  inline i32 font_atlas_initial_size = 1024;
  inline i32 font_atlas_max_size = 4096;
  inline i32 font_min_pixel_height = 4;
  inline i32 font_max_pixel_height = 256;
  inline i32 texture_atlas_size = 1024;
  inline i32 texture_atlas_margin = 4;
  inline i32 texture_atlas_grid_size = 4;
  inline i32 texture_atlas_max_alias_depth = 16;

} // namespace zincgui::default_settings
