/**
 * @file ModernTimelineRuler.h
 * @brief Professional timeline ruler with grid and markers
 * @author Fixed by Claude - December 2025
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ZenithTheme.h"
#include "../engine/EngineConstants.h"

namespace zenith {

/**
 * @class ModernTimelineRuler
 * @brief Timeline ruler showing time, beats, and grid divisions
 */
class ModernTimelineRuler : public juce::Component {
public:
    ModernTimelineRuler();
    ~ModernTimelineRuler() override = default;
    
    //==========================================================================
    // Component Overrides
    //==========================================================================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    //==========================================================================
    // Timeline Control
    //==========================================================================
    void setPixelsPerBeat(double ppb);
    double getPixelsPerBeat() const { return pixelsPerBeat_; }
    
    void setViewportStartBeat(double beat);
    double getViewportStartBeat() const { return viewportStartBeat_; }
    
    void setTimeSignature(int numerator, int denominator);
    void setTempo(double bpm);

    void setSampleRate(double sampleRate);
    void setFrameRate(double fps);
    
    void setPlayheadPosition(double beat);
    double getPlayheadPosition() const { return playheadBeat_; }
    
    void setLoopRegion(double startBeat, double endBeat);
    void clearLoopRegion();
    bool hasLoopRegion() const { return loopEnabled_; }
    
    //==========================================================================
    // Display Options
    //==========================================================================
    enum class TimeFormat {
        Bars,           // 1.1.1 (bar.beat.tick)
        Time,           // 00:00:000 (minutes:seconds:milliseconds)
        Samples,        // Sample count
        Frames          // Frame count (for video)
    };
    
    void setTimeFormat(TimeFormat format);
    TimeFormat getTimeFormat() const { return timeFormat_; }
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    std::function<void(double)> onPlayheadMoved;
    std::function<void(double, double)> onLoopRegionChanged;
    
private:
    //==========================================================================
    // Drawing Helpers
    //==========================================================================
    void drawRulerBackground(juce::Graphics& g);
    void drawGridLines(juce::Graphics& g);
    void drawTimeMarkers(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);
    
    juce::String formatTime(double beat);
    int beatToPixel(double beat);
    double pixelToBeat(int pixel);
    
    //==========================================================================
    // State
    //==========================================================================
    double pixelsPerBeat_ = 40.0;
    double viewportStartBeat_ = 0.0;
    double playheadBeat_ = 0.0;
    
    int timeSignatureNumerator_ = 4;
    int timeSignatureDenominator_ = 4;
    double tempo_ = 120.0;

    double sampleRate_ = constants::kDefaultSampleRate;
    double fps_ = constants::kDefaultFrameRate;
    
    bool loopEnabled_ = false;
    double loopStartBeat_ = 0.0;
    double loopEndBeat_ = 0.0;
    
    TimeFormat timeFormat_ = TimeFormat::Bars;
    
    bool isDraggingPlayhead_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernTimelineRuler)
};

} // namespace zenith
