#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"
#include <core/SkBitmap.h>
#include <string>

namespace zenith::industrial {

class CarbonPanel : public IndustrialComponent {
public:
  CarbonPanel(std::string title,
              IndustrialTheme& theme,
              const SkBitmap* texture,
              bool expertDecorations);

  void render(SkCanvas* canvas) override;

private:
  std::string title_;
  IndustrialTheme& theme_;
  const SkBitmap* texture_ = nullptr;
  bool expertDecorations_ = false;
};

} // namespace zenith::industrial
