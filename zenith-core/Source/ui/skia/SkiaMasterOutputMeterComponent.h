/**
 * @file SkiaMasterOutputMeterComponent.h
 * @brief GPU-rendered master output level meter with professional ballistics
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "SkiaCanvasComponent.h"
#include "SkiaTheme.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaMasterOutputMeterComponent : public SkiaCanvasComponent, private juce::Timer
{
public:
    SkiaMasterOutputMeterComponent();
    ~SkiaMasterOutputMeterComponent() override;

    void setLevel(float leftLevel, float rightLevel);
    void setPeakLevel(float leftPeak, float rightPeak);

protected:
    void paintSkia(SkCanvas& canvas, const juce::Rectangle<int>& bounds) override;

private:
    void timerCallback() override;
    void renderMeter(SkCanvas& canvas, float x, float y, float width, float height,
                     float level, float peak, const juce::String& label);

    // Current levels (set from audio thread via atomic-safe setLevel)
    float leftLevel_ = 0.0f;
    float rightLevel_ = 0.0f;
    float leftPeak_ = 0.0f;
    float rightPeak_ = 0.0f;

    // Smoothed for display (VU-style ballistics ~300ms)
    float leftSmoothed_ = 0.0f;
    float rightSmoothed_ = 0.0f;

    // Peak hold for visual feedback
    float leftPeakHold_ = 0.0f;
    float rightPeakHold_ = 0.0f;
    int leftPeakHoldTimer_ = 0;
    int rightPeakHoldTimer_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMasterOutputMeterComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
