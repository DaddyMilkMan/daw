#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"
#include <vector>

namespace zenith::industrial {

class VisualizerDisplay : public IndustrialComponent {
public:
  explicit VisualizerDisplay(IndustrialTheme& theme);

  void render(SkCanvas* canvas) override;
  void tick();

private:
  IndustrialTheme& theme_;
  std::vector<float> samples_;
  float phase_ = 0.0f;
};

} // namespace zenith::industrial
