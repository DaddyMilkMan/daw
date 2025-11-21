/**
 * @file SkiaTransportControlComponent.h
 * @brief GPU-rendered transport controls with Skia
 *
 * Features:
 * - Play/Stop/Record buttons with LED indicators
 * - Tempo display with tap tempo
 * - Timeline position indicator
 * - Loop controls
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SkiaTransportControlComponent : public juce::Component
{
public:
    SkiaTransportControlComponent();
    ~SkiaTransportControlComponent() override = default;

    void setIsPlaying(bool playing);
    bool getIsPlaying() const { return isPlaying_; }

    void setIsRecording(bool recording);
    bool getIsRecording() const { return isRecording_; }

    void setTempo(float bpm);
    float getTempo() const { return tempo_; }

    void setTimelinePosition(double seconds);
    double getTimelinePosition() const { return timelinePosition_; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    bool isPlaying_ = false;
    bool isRecording_ = false;
    bool isLooping_ = false;
    float tempo_ = 120.0f;
    double timelinePosition_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaTransportControlComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
