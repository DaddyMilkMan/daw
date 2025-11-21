/**
 * @file SkiaMasterOutputMeterComponent.h
 * @brief GPU-rendered master output level meter
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaMasterOutputMeterComponent : public juce::Component, private juce::Timer
{
public:
    SkiaMasterOutputMeterComponent();
    ~SkiaMasterOutputMeterComponent() override;

    void setLevel(float leftLevel, float rightLevel);
    void setPeakLevel(float leftPeak, float rightPeak);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    float leftLevel_ = 0.0f;
    float rightLevel_ = 0.0f;
    float leftPeak_ = 0.0f;
    float rightPeak_ = 0.0f;
    float leftSmoothed_ = 0.0f;
    float rightSmoothed_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMasterOutputMeterComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
