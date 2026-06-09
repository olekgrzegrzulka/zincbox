#pragma once
#include <glm/ext/vector_float2.hpp>
#include "button.hpp"
#include "common/types.hpp"
#include "sprite.hpp"
#include "widget.hpp"

class UI;

enum class SliderOrientation {
  HORIZONTAL,
  VERTICAL,
};

enum class ThumbConstraint {
  INSIDE_TRACK,
  FULL_RANGE,
};

class Slider : public Widget {
  protected:
    Sprite& track;
    Sprite& track_active;
    Sprite& thumb;

    ThumbConstraint thumb_constraint = ThumbConstraint::INSIDE_TRACK;
    SliderOrientation orientation = SliderOrientation::HORIZONTAL;
    i32 track_thickness = 10;
    i32 thumb_length = 10;
    i32 thumb_thickness = 10;
    i32 drag_area_inflation = 0;

    float value = 0.0f;
    float old_value = 0.0f;
    float min_value = -10.0f;
    float max_value = 10.0f;

    bool is_dragged = false;
    std::function<void(float, float)> lambda_value_changed = nullptr;
    std::function<void(float, float)> lambda_drag_end = nullptr;

    i32 drag_start_mouse_pos = 0;
    i32 drag_start_thumb_pos = 0;
    float drag_start_value = 0.0;

    vec2f uv_start_thumb_idle{};
    vec2f uv_end_thumb_idle{};
    vec2f uv_start_thumb_hovered{};
    vec2f uv_end_thumb_hovered{};
    vec2f uv_start_thumb_pressed{};
    vec2f uv_end_thumb_pressed{};
    vec2f uv_start_track_inactive{};
    vec2f uv_end_track_inactive{};
    vec2f uv_start_track_active{};
    vec2f uv_end_track_active{};

  public:
    Slider(UI& ui_, SliderOrientation orientation_ = SliderOrientation::HORIZONTAL);

    void on_value_changed(std::function<void(float, float)> a) { lambda_value_changed = a; }
    void on_drag_ended(std::function<void(float, float)> a) { lambda_drag_end = a; }
    bool is_being_dragged() { return is_dragged; }

    void update() override;
    void draw() override { Widget::draw(); }

    void event(Input::InputEventMouseButton&) override;
    void event(Input::InputEventMouseMove&) override;
    void event(Input::InputEventMouseScroll&) override;
    void event(Input::InputEventKey&) override;
    void event(Input::InputEventMouseEntered&) override;
    void event(Input::InputEventMouseLeft&) override;
    void event(Input::InputEventCloseWindow&) override;

    WIDGET_DEF_GETTER(value)
    WIDGET_DEF_GETTER(min_value)
    WIDGET_DEF_GETTER(max_value)
    WIDGET_DEF_SETTER(drag_area_inflation)
    WIDGET_DEF_GETTER(drag_area_inflation)

    const Sprite& get_thumb() const { return thumb; }

    void set_value(float new_value, bool signal = true) {
      value = new_value;
      if (!signal) { old_value = new_value; }
    }
    WIDGET_DEF_SETTER_DIRTY(min_value)
    WIDGET_DEF_SETTER_DIRTY(max_value)

    WIDGET_DEF_SETTER_DIRTY(thumb_constraint)
    WIDGET_DEF_SETTER_DIRTY(orientation)
    WIDGET_DEF_SETTER_DIRTY(track_thickness)
    WIDGET_DEF_SETTER_DIRTY(thumb_length)
    WIDGET_DEF_SETTER_DIRTY(thumb_thickness)

    void set_texture_thumb_pressed(const std::string& id);
    void set_texture_thumb_hovered(const std::string& id);
    void set_texture_thumb_idle(const std::string& id);
    void set_texture_track_inactive(const std::string& id);
    void set_texture_track_active(const std::string& id);

    float track_nine_slice_margin = 6.0f;
    float thumb_nine_slice_margin = 6.0f;
    WIDGET_DEF_SETTER_DIRTY(track_nine_slice_margin)
    WIDGET_DEF_SETTER_DIRTY(thumb_nine_slice_margin)
};
