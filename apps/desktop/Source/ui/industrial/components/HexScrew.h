#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"

namespace zenith::industrial {

class HexScrew : public IndustrialComponent {
public:
  explicit HexScrew(IndustrialTheme& theme) : theme_(theme) {}
  void render(SkCanvas* canvas) override;

private:
  IndustrialTheme& theme_;
};

} // namespace zenith::industrial
