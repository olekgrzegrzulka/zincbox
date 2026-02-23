#include "scrollbar.hpp"
#include <algorithm>
#include <cmath>
#include "../input.hpp"
#include "../types.hpp"
#include "sprite.hpp"
#include "widget.hpp"

void ScrollBar::update() {
  using enum ScrollBarOrientation;

  bool lmb_pressed = Input::mouse_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb_just_pressed = Input::mouse_just_pressed(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool lmb_just_released = Input::mouse_just_released(Input::MouseButton::MOUSE_BUTTON_LEFT);
  bool mouse_on_thumb = thumb.is_mouse_hovering();
  bool mouse_on_track = track.is_mouse_hovering();

  auto document_pos_to_scrollbar_pos = [&](i32 document_pos) {
    if (orientation == HORIZONTAL) {
      return std::clamp<i32>(
        (i32)(document_pos / (double)(content_size - page_size) * (width - thumb.get_width())),
        0, width - thumb.get_width());
    } else {
      return std::clamp<i32>(
        (i32)(document_pos / (double)(content_size - page_size) * (height - thumb.get_height())),
        0, height - thumb.get_height());
    }
  };

  auto scrollbar_pos_to_document_pos = [&](i32 scrollbar_pos) {
    if (orientation == HORIZONTAL) {
      return std::clamp<i32>(
        (i32)(scrollbar_pos / (double)(width - thumb.get_width()) * (content_size - page_size)),
        0, content_size - page_size);
    } else {
      return std::clamp<i32>(
        (i32)(scrollbar_pos / (double)(height - thumb.get_height()) * (content_size - page_size)),
        0, content_size - page_size);
    }
  };

  if (lmb_just_pressed && !mouse_on_thumb && mouse_on_track) {
    if (orientation == HORIZONTAL) {
      i32 thumb_x = (Input::get_mouse_x() - get_position().x) - thumb.get_width() / 2;
      scroll_offset = scrollbar_pos_to_document_pos(thumb_x);
      thumb_x = std::clamp(thumb_x, 0, width - thumb.get_width());
      thumb.set_x(thumb_x);
      mouse_on_thumb = true;
    } else {
      i32 thumb_y = (Input::get_mouse_y() - get_position().y) - thumb.get_height() / 2;
      scroll_offset = scrollbar_pos_to_document_pos(thumb_y);
      thumb_y = std::clamp(thumb_y, 0, height - thumb.get_height());
      thumb.set_y(thumb_y);
      mouse_on_thumb = true;
    }
  }

  if (lmb_just_pressed && mouse_on_thumb) {
    is_dragged = true;
    drag_start_scroll_offset = scroll_offset;
    if (orientation == HORIZONTAL) {
      drag_start_mouse_pos = Input::get_mouse_x();
      drag_start_thumb_pos = thumb.get_x();
    } else {
      drag_start_mouse_pos = Input::get_mouse_y();
      drag_start_thumb_pos = thumb.get_y();
    }
  }

  if (lmb_just_released) {
    is_dragged = false;
  }

  if (orientation == HORIZONTAL) {
    track.set_width(width);
    if (content_size != 0 && width != 0) {
      i32 thumb_width = std::max(16.0, page_size / (double)content_size * width);
      thumb.set_width(thumb_width);
    }

    if (is_dragged) {
      i32 scrollbar_drag = Input::get_mouse_x() - drag_start_mouse_pos;
      i32 document_drag = scrollbar_drag / (double)(width - thumb.get_width()) * (content_size - page_size);

      scroll_offset = drag_start_scroll_offset + document_drag;
      scroll_offset = std::clamp(scroll_offset, 0, content_size - page_size);

      i32 thumb_x = drag_start_thumb_pos + scrollbar_drag;
      thumb_x = std::clamp(thumb_x, 0, width - thumb.get_width());
      thumb.set_x(thumb_x);
    } else {
      i32 thumb_x = document_pos_to_scrollbar_pos(scroll_offset);
      thumb.set_x(thumb_x);
    }
  } else {
    track.set_height(height);
    if (content_size != 0 && height != 0) {
      i32 thumb_height = std::max(16.0, page_size / (double)content_size * height);
      thumb.set_height(thumb_height);
    }

    if (is_dragged) {
      i32 scrollbar_drag = Input::get_mouse_y() - drag_start_mouse_pos;
      i32 document_drag = scrollbar_drag / (double)(height - thumb.get_height()) * (content_size - page_size);

      scroll_offset = drag_start_scroll_offset + document_drag;
      scroll_offset = std::clamp(scroll_offset, 0, content_size - page_size);

      i32 thumb_y = drag_start_thumb_pos + scrollbar_drag;
      thumb_y = std::clamp(thumb_y, 0, height - thumb.get_height());
      thumb.set_y(thumb_y);
    } else {
      i32 thumb_y = document_pos_to_scrollbar_pos(scroll_offset);
      thumb.set_y(thumb_y);
    }
  }

  if (is_dragged) {
    thumb.set_texture("scrollbar_thumb_pressed", false);
  } else if (mouse_on_thumb && !lmb_pressed) {
    thumb.set_texture("scrollbar_thumb_hovered", false);
  } else {
    thumb.set_texture("scrollbar_thumb_idle", false);
  }

  if (old_scroll_offset != scroll_offset) {
    old_scroll_offset = scroll_offset;
    if (lambda) {
      lambda(scroll_offset);
    }
  }

  Widget::update();
}

void ScrollBar::scroll(float force) {
  scroll_offset -= force * 40;
  scroll_offset = std::clamp(scroll_offset, 0, content_size - page_size);
}

void ScrollBar::handle_event(Input::InputEventMouseButton&) {
}
void ScrollBar::handle_event(Input::InputEventMouseMove&) {
}
void ScrollBar::handle_event(Input::InputEventMouseScroll& ev) {
  if (!is_dragged && is_mouse_hovering()) {
    scroll(ev.offset.y);
    ev.handled = true;
  }
}
void ScrollBar::handle_event(Input::InputEventKey&) {
}
void ScrollBar::handle_event(Input::InputEventMouseEntered&) {
}
void ScrollBar::handle_event(Input::InputEventMouseLeft&) {
}
void ScrollBar::handle_event(Input::InputEventCloseWindow&) {
}