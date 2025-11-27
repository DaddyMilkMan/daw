/**
 * @file SkiaArrangementViewComponent.h
 * @brief Logic Pro style arrangement/timeline view
 *
 * Features:
 * - Grid lines (bars and beats)
 * - Playhead with triangle caps
 * - Audio/MIDI regions with waveforms
 * - Ruler with bar numbers
 * - Loop/cycle region
 * - Snap guides
 */

#pragma once

#include <JuceHeader.h>

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

struct AudioRegion
{
    juce::String name;
    float startTime;      // In beats
    float duration;       // In beats
    int trackIndex;
    juce::Colour color;
    bool isMuted;
    std::vector<float> waveform;  // Normalized -1 to 1
};

class SkiaArrangementViewComponent : public juce::Component
{
public:
    SkiaArrangementViewComponent();
    ~SkiaArrangementViewComponent() override = default;

    // Playback
    void setPlayheadPosition(float beats);
    float getPlayheadPosition() const { return playheadPosition_; }
    
    // Loop region
    void setLoopEnabled(bool enabled);
    void setLoopRange(float startBeats, float endBeats);
    
    // Regions
    void addRegion(const AudioRegion& region);
    void clearRegions();
    
    // View control
    void setZoom(float pixelsPerBeat);
    void setScrollPosition(float beats);
    
    // Grid
    void setTimeSignature(int numerator, int denominator);
    void setTempo(float bpm);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    // Playback state
    float playheadPosition_ = 0.0f;  // In beats
    
    // Loop
    bool loopEnabled_ = false;
    float loopStart_ = 0.0f;
    float loopEnd_ = 16.0f;
    
    // View state
    float pixelsPerBeat_ = 40.0f;   // Zoom level
    float scrollPosition_ = 0.0f;    // Horizontal scroll in beats
    
    // Time signature
    int timeSigNumerator_ = 4;
    int timeSigDenominator_ = 4;
    float tempo_ = 120.0f;
    
    // Regions
    std::vector<AudioRegion> regions_;
    
    // UI constants
    static constexpr int rulerHeight_ = 24;
    
    // Helper methods
    float beatsToPixels(float beats) const;
    float pixelsToBeats(float pixels) const;
    void drawRuler(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawRegions(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& totalBounds);
    void drawLoopRegion(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaArrangementViewComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
