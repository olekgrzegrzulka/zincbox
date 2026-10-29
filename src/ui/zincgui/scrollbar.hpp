#pragma once
#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include "common/types.hpp"
#include "slider.hpp"
#include "ui/zincgui/default_settings.hpp"

namespace zincgui {

  class Root;

  class ScrollBar : public Slider {
    public:
      ScrollBar(Root& ui_, SliderOrientation orientation_ = SliderOrientation::HORIZONTAL);

      void update() override;

      void recalculate_values();
      void scroll(float force);

      [[nodiscard]] i32 content_size() const noexcept { return m_content_size; }
      ScrollBar& content_size(i32 v) {
        if (m_content_size != v) {
          m_content_size = v;
          recalculate_values();
        }
        return *this;
      }

      [[nodiscard]] i32 page_size() const noexcept { return m_page_size; }
      ScrollBar& page_size(i32 v) {
        if (m_page_size != v) {
          m_page_size = v;
          recalculate_values();
        }
        return *this;
      }

      [[nodiscard]] i32 scroll_offset() const noexcept { return static_cast<i32>(value()); }
      ScrollBar& scroll_offset(i32 v) {
        float lo = std::min(min_value(), max_value());
        float hi = std::max(min_value(), max_value());
        value(std::clamp(static_cast<float>(v), lo, hi));
        recalculate_values();
        return *this;
      }

    private:
      i32 m_page_size = default_settings::scrollbar_page_size;
      i32 m_content_size = default_settings::scrollbar_content_size;
  };

} // namespace zincgui
