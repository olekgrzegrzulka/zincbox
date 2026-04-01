#pragma once
#include <glm/ext/vector_float2.hpp>
#include "button.hpp"
#include "common/types.hpp"
#include "slider.hpp"
#include "sprite.hpp"
#include "widget.hpp"

class UI;

class ScrollBar : public Slider {
  protected:
    i32 page_size = 1;
    i32 content_size = 1;

  public:
    ScrollBar(UI& ui_, SliderOrientation orientation_ = SliderOrientation::HORIZONTAL);

    void update() override;
    void draw() override;
    void recalculate_values();

    void scroll(float force);

    void handle_event(Input::InputEventMouseScroll&) override;

    void set_content_size(i32 v) {
      content_size = v;
      recalculate_values();
    }
    void set_page_size(i32 v) {
      page_size = v;
      recalculate_values();
    }

    void set_scroll_offset(i32 v) {
      set_value(std::clamp((float)v, min_value, max_value));
      recalculate_values();
    }
};
