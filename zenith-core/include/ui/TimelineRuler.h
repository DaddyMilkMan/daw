/**
 * @file TimelineRuler.h
 * @brief Timeline ruler showing bars, beats, and playhead
 *
 * Arranger Timeline UI - Phase 14
 * Displays the horizontal timeline with bar/beat markers
 */

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * @class TimelineRuler
 * @brief Displays bars, beats, and playhead position
 *
 * Features:
 * - Bar and beat markers
 * - Playhead indicator
 * - Beat-based horizontal scrolling
 * - Simple grid (no TempoMap yet)
 */
class TimelineRuler : public juce::Component,
                      private juce::Timer
{
public:
    //==========================================================================
    TimelineRuler();
    ~TimelineRuler() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timeline control
    //==========================================================================

    /**
     * @brief Set horizontal scale (pixels per beat)
     */
    void setPixelsPerBeat(double ppb);

    /**
     * @brief Get pixels per beat
     */
    double getPixelsPerBeat() const { return pixelsPerBeat; }

    /**
     * @brief Set horizontal scroll offset in pixels
     */
    void setScrollOffset(int offset);

    /**
     * @brief Get horizontal scroll offset in pixels
     */
    int getScrollOffset() const { return scrollOffset; }

    /**
     * @brief Set time signature
     */
    void setTimeSignature(int numerator, int denominator);

    /**
     * @brief Set playhead position in beats
     */
    void setPlayheadPosition(double positionInBeats);

    /**
     * @brief Get playhead position in beats
     */
    double getPlayheadPosition() const { return playheadBeats; }

private:
    //==========================================================================
    // Timer interface (for playhead updates)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Member variables
    //==========================================================================

    double pixelsPerBeat = 40.0;        // Horizontal scale
    int scrollOffset = 0;                // Horizontal scroll in pixels
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    double playheadBeats = 0.0;          // Current playhead position

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};
