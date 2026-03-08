#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"

namespace zenith::industrial {

class IndustrialToggle : public IndustrialComponent {
public:
  explicit IndustrialToggle(IndustrialTheme& theme);

  void render(SkCanvas* canvas) override;
  void handleMouseUp(const MouseEvent& e) override;

  bool isOn() const { return on_; }
  void setOn(bool on) { on_ = on; }

private:
  bool on_ = false;
  IndustrialTheme& theme_;
};

} // namespace zenith::industrial
