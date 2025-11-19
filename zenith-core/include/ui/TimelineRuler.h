/**
 * @file TimelineRuler.h
 * @brief Timeline ruler showing beat markers
 */

#pragma once

#include <JuceHeader.h>

/**
 * @class TimelineRuler
 * @brief Displays a horizontal timeline with beat markers
 *
 * Shows beat numbers and grid lines based on zoom level.
 * Works in beat units, independent of tempo/sample rate.
 */
class TimelineRuler : public juce::Component
{
public:
    TimelineRuler();
    ~TimelineRuler() override = default;

    //==========================================================================
    // View control
    //==========================================================================

    /**
     * @brief Set the visible range in beats
     * @param start Start beat
     * @param length Number of beats visible
     */
    void setVisibleRange(double start, double length);

    /**
     * @brief Get pixels per beat ratio
     */
    double getPixelsPerBeat() const { return pixelsPerBeat; }

    /**
     * @brief Convert beats to pixels
     */
    int beatsToPixels(double beats) const;

    /**
     * @brief Convert pixels to beats
     */
    double pixelsToBeats(int pixels) const;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    double viewStartBeat = 0.0;
    double viewLengthBeats = 32.0;
    double pixelsPerBeat = 20.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};
