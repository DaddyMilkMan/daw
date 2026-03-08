#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"

namespace zenith::industrial {

class IndustrialLED : public IndustrialComponent {
public:
  explicit IndustrialLED(IndustrialTheme& theme);

  void render(SkCanvas* canvas) override;
  void setActive(bool active) { active_ = active; }

private:
  bool active_ = false;
  IndustrialTheme& theme_;
};

} // namespace zenith::industrial
