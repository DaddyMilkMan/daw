#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"
#include <functional>
#include <string>

namespace zenith::industrial {

class IndustrialKnob : public IndustrialComponent {
public:
  enum class Size {
    Primary,
    Standard,
    Small
  };

  IndustrialKnob(std::string label,
                 std::string unit,
                 float initialValue,
                 IndustrialTheme& theme,
                 Size size = Size::Standard,
                 bool expertDecorations = false);

  void render(SkCanvas* canvas) override;
  void handleMouseDown(const MouseEvent& e) override;
  void handleMouseDrag(const MouseEvent& e) override;
  void handleMouseUp(const MouseEvent& e) override;
  void handleMouseMove(const MouseEvent& e) override;

  float getValue() const { return value_; }
  void setValue(float value);

private:
  std::string label_;
  std::string unit_;
  float value_ = 0.5f;
  float dragStartValue_ = 0.5f;
  float dragStartY_ = 0.0f;
  bool showTooltip_ = false;
  Size size_ = Size::Standard;
  bool expertDecorations_ = false;
  IndustrialTheme& theme_;
};

} // namespace zenith::industrial
