#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include "common/types.hpp"
#include "ui/zincgui/default_settings.hpp"
#include "ui/zincgui/sprite.hpp"
#include "ui/zincgui/widget.hpp"

namespace zincgui {

  class Root;

  enum class SliderOrientation : u8 { HORIZONTAL, VERTICAL, HORIZONTAL_INVERTED, VERTICAL_INVERTED };

  enum class ThumbConstraint : u8 { INSIDE_TRACK, FULL_RANGE };

  class Slider : public Widget {
    public:
      enum class SliderState : u8 { IDLE, PRESSED, HOVERED };

      Slider(Root& ui_, SliderOrientation orientation_ = SliderOrientation::HORIZONTAL);
      ~Slider() override = default;

      void input() override;
      void update() override;
      void event(Input::InputEventMouseButton&) override;
      void event(Input::InputEventMouseMove&) override;
      void event(Input::InputEventMouseScroll&) override;
      void event(Input::InputEventKey&) override;

      void skip_anim();
      [[nodiscard]] bool is_being_dragged() const noexcept { return m_is_dragged; }

      [[nodiscard]] Sprite& thumb() noexcept { return m_thumb; }
      [[nodiscard]] const Sprite& thumb() const noexcept { return m_thumb; }
      [[nodiscard]] Sprite& track() noexcept { return m_track; }
      [[nodiscard]] const Sprite& track() const noexcept { return m_track; }
      [[nodiscard]] Sprite& track_active() noexcept { return m_track_active; }
      [[nodiscard]] const Sprite& track_active() const noexcept { return m_track_active; }

      template <typename Self> Self& on_value_changed(this Self& self, std::function<void(float, float)> lambda) {
        self.m_on_value_changed = std::move(lambda);
        return self;
      }

      template <typename Self> Self& on_drag_ended(this Self& self, std::function<void(float, float)> lambda) {
        self.m_on_drag_ended = std::move(lambda);
        return self;
      }

      [[nodiscard]] float value() const noexcept { return m_value; }
      template <typename Self> Self& value(this Self& self, float new_value, bool signal = true) {
        i32 old_value = self.m_value;
        float lo = std::min(self.m_min_value, self.m_max_value);
        float hi = std::max(self.m_min_value, self.m_max_value);
        new_value = std::clamp(new_value, lo, hi);
        if (self.m_value == new_value) { return self; }

        self.m_value = new_value;
        if (signal && self.m_on_value_changed) { self.m_on_value_changed(old_value, self.m_value); }
        return self;
      }

      [[nodiscard]] float min_value() const noexcept { return m_min_value; }
      template <typename Self> Self& min_value(this Self& self, float v) {
        if (self.m_min_value != v) {
          self.m_min_value = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float max_value() const noexcept { return m_max_value; }
      template <typename Self> Self& max_value(this Self& self, float v) {
        if (self.m_max_value != v) {
          self.m_max_value = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float sensitivity() const noexcept { return m_sensitivity; }
      template <typename Self> Self& sensitivity(this Self& self, float v) {
        if (self.m_sensitivity != v) { self.m_sensitivity = v; }
        return self;
      }

      [[nodiscard]] i32 drag_area_inflation() const noexcept { return m_drag_area_inflation; }
      template <typename Self> Self& drag_area_inflation(this Self& self, i32 v) {
        if (self.m_drag_area_inflation != v) { self.m_drag_area_inflation = v; }
        return self;
      }

      [[nodiscard]] bool disabled() const noexcept { return m_disabled; }
      template <typename Self> Self& disabled(this Self& self, bool v) {
        if (self.m_disabled != v) {
          self.m_disabled = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] bool is_scrollable() const noexcept { return m_is_scrollable; }
      template <typename Self> Self& is_scrollable(this Self& self, bool v) {
        if (self.m_is_scrollable != v) { self.m_is_scrollable = v; }
        return self;
      }

      [[nodiscard]] ThumbConstraint thumb_constraint() const noexcept { return m_thumb_constraint; }
      template <typename Self> Self& thumb_constraint(this Self& self, ThumbConstraint v) {
        if (self.m_thumb_constraint != v) {
          self.m_thumb_constraint = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] SliderOrientation orientation() const noexcept { return m_orientation; }
      Slider& orientation(SliderOrientation o);

      [[nodiscard]] i32 track_thickness() const noexcept { return m_track_thickness; }
      template <typename Self> Self& track_thickness(this Self& self, i32 v) {
        if (self.m_track_thickness != v) {
          self.m_track_thickness = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 thumb_length() const noexcept { return m_thumb_length; }
      template <typename Self> Self& thumb_length(this Self& self, i32 v) {
        if (self.m_thumb_length != v) {
          self.m_thumb_length = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] i32 thumb_thickness() const noexcept { return m_thumb_thickness; }
      template <typename Self> Self& thumb_thickness(this Self& self, i32 v) {
        if (self.m_thumb_thickness != v) {
          self.m_thumb_thickness = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float track_nine_slice_margin() const noexcept { return m_track_nine_slice_margin; }
      template <typename Self> Self& track_nine_slice_margin(this Self& self, float v) {
        if (self.m_track_nine_slice_margin != v) {
          self.m_track_nine_slice_margin = v;
          self.mark_dirty();
        }
        return self;
      }

      [[nodiscard]] float thumb_nine_slice_margin() const noexcept { return m_thumb_nine_slice_margin; }
      template <typename Self> Self& thumb_nine_slice_margin(this Self& self, float v) {
        if (self.m_thumb_nine_slice_margin != v) {
          self.m_thumb_nine_slice_margin = v;
          self.mark_dirty();
        }
        return self;
      }

      Slider& texture_thumb_pressed(const std::string& id);
      Slider& texture_thumb_hovered(const std::string& id);
      Slider& texture_thumb_idle(const std::string& id);
      Slider& texture_track_inactive(const std::string& id);
      Slider& texture_track_active(const std::string& id);

    private:
      Sprite& m_track;
      Sprite& m_track_active;
      Sprite& m_thumb;

      SliderState m_state = SliderState::IDLE;
      SliderState m_state_old = static_cast<SliderState>(-1);

      ThumbConstraint m_thumb_constraint = ThumbConstraint::INSIDE_TRACK;
      SliderOrientation m_orientation = SliderOrientation::HORIZONTAL;
      i32 m_track_thickness = default_settings::slider_track_thickness;
      i32 m_thumb_length = default_settings::slider_thumb_length;
      i32 m_thumb_thickness = default_settings::slider_thumb_thickness;
      i32 m_drag_area_inflation = default_settings::slider_drag_area_inflation;

      float m_value = 0.0f;
      float m_min_value = default_settings::slider_min_value;
      float m_max_value = default_settings::slider_max_value;
      float m_norm_pos_anim = 0.0f;
      float m_sensitivity = default_settings::slider_sensitivity;
      bool m_disabled = false;
      bool m_is_dragged = false;
      bool m_is_scrollable = true;
      std::function<void(float, float)> m_on_value_changed;
      std::function<void(float, float)> m_on_drag_ended;

      i32 m_drag_start_mouse_pos = 0;
      i32 m_drag_start_thumb_pos = 0;
      float m_drag_start_value = 0.0f;

      vec2f m_uv_start_thumb_idle{};
      vec2f m_uv_end_thumb_idle{};
      vec2f m_uv_start_thumb_hovered{};
      vec2f m_uv_end_thumb_hovered{};
      vec2f m_uv_start_thumb_pressed{};
      vec2f m_uv_end_thumb_pressed{};
      vec2f m_uv_start_track_inactive{};
      vec2f m_uv_end_track_inactive{};
      vec2f m_uv_start_track_active{};
      vec2f m_uv_end_track_active{};

      float m_track_nine_slice_margin = default_settings::slider_track_nine_slice_margin;
      float m_thumb_nine_slice_margin = default_settings::slider_thumb_nine_slice_margin;
  };

} // namespace zincgui
