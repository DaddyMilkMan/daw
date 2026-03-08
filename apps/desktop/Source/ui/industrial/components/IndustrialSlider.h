#pragma once

#include "../rendering/IndustrialTheme.h"
#include "IndustrialComponent.h"
#include <string>

namespace zenith::industrial {

class IndustrialSlider : public IndustrialComponent {
public:
  IndustrialSlider(std::string label, float initialValue,
                   IndustrialTheme &theme);

  void render(SkCanvas *canvas) override;
  void handleMouseDown(const MouseEvent &e) override;
  void handleMouseDrag(const MouseEvent &e) override;
  void handleMouseUp(const MouseEvent &e) override;

private:
  std::string label_;
  float value_ = 0.5f;
  float dragStartValue_ = 0.5f;
  float dragStartY_ = 0.0f;
  bool dragging_ = false;
  IndustrialTheme &theme_;
};

} // namespace zenith::industrial
