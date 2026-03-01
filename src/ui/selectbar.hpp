#pragma once
#include <functional>
#include <string>
#include <vector>
#include "button.hpp"
#include "sprite.hpp"
#include "ui.hpp"

class SelectBar : public Sprite {
  public:
    enum class Orientation {
      HORIZONTAL,
      VERTICAL
    };

  protected:
    Sprite& highlight;
    std::vector<Button*> options;
    i32 selected_index = 0;
    std::function<void(i32)> lambda_change = nullptr;

    float old_x = 0.0f;
    float old_y = 0.0f;
    float old_w = 0.0f;
    float old_h = 0.0f;
    float anim_progress = 1.0f;

  public:
    SelectBar(UI& ui_, Orientation orientation = Orientation::HORIZONTAL);

    void update() override;
    Button& add_option(std::string label);
    void select(i32 index, bool animation = true);
    i32 get_selected_index() const { return selected_index; }
    void on_change(std::function<void(i32)> lambda_) {
      lambda_change = lambda_;
    }
    Sprite& get_highlight() { return highlight; }
};