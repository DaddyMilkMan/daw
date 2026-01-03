/*
  ==============================================================================

    ZenithTooltipOverlay.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Glass overlay for Learning Mode / tooltip display.
    Highlights a target control with glow and shows tooltip card.

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "ZenithControl.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#endif

namespace zenith {

class ZenithTooltipOverlay : public SkiaComponent {
public:
  ZenithTooltipOverlay() = default;
  ~ZenithTooltipOverlay() override = default;

  // ----- Target -----
  void setTarget(ZenithControl *target);
  ZenithControl *getTarget() const { return target_; }

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

  // Pass through mouse events
  bool hitTest(int x, int y) override;

private:
  void drawTooltipCard(SkCanvas *canvas, const SkRect &targetRect);

  ZenithControl *target_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTooltipOverlay)
};

} // namespace zenith
