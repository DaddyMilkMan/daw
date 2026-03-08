#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"
#include <functional>
#include <string>

namespace zenith::industrial {

class IndustrialButton : public IndustrialComponent {
public:
  enum class Shape {
    Square,
    Pill,
    Circle
  };

  IndustrialButton(std::string label,
                   IndustrialTheme& theme,
                   Shape shape = Shape::Square,
                   bool toggle = true);

  void render(SkCanvas* canvas) override;
  void handleMouseDown(const MouseEvent& e) override;
  void handleMouseUp(const MouseEvent& e) override;

  bool isOn() const { return isOn_; }
  void setOn(bool on) { isOn_ = on; }
  void setLabel(std::string label) { label_ = std::move(label); }

  std::function<void(bool)> onToggle;

private:
  std::string label_;
  IndustrialTheme& theme_;
  Shape shape_;
  bool toggle_ = true;
  bool isOn_ = false;
};

} // namespace zenith::industrial
