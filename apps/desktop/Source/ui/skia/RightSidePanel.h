/*
  ==============================================================================

    RightSidePanel.h
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

    Layout container for Wingman console and scratch pads.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

class RightSidePanel : public SkiaComponent {
public:
    RightSidePanel();
    ~RightSidePanel() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void timerCallback() override;

private:
    // Cached resources for 60FPS rendering
    SkPaint bgPaint_;
    SkPaint borderPaint_;
    SkPaint meterBgPaint_;
    SkPaint meterPeakPaint_;
    SkPaint meterRmsPaint_;
    SkPaint textPaint_;
    SkPaint subTextPaint_;
    SkFont headerFont_;
    SkFont bodyFont_;
    SkFont labelFont_;
    SkRect cachedBounds_;
    
    float animationPhase_ = 0.0f;
    
    void updateCachedPaints(const SkRect& bounds);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
