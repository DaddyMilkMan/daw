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
 * Features Apple-inspired design with gradients, hover feedback, and smooth animations.
 */
class TimelineRuler : public juce::Component,
                      public juce::Timer
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
    void setVisibleRange(double start, double length) [[maybe_unused]];

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

    /**
     * @brief Set callback for seek requests
     */
    std::function<void(double)> onSeek;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void timerCallback() override;

private:
    // View state
    double viewStartBeat = 0.0;
    double viewLengthBeats = 32.0;
    double pixelsPerBeat = 20.0;

    // Hover state
    bool isHovered = false;
    int hoveredMeasure = -1;
    juce::Point<int> mousePosition;
    float hoverAnimation = 0.0f;

    // Helper methods
    void drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawBeatMarkers(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawHoverFeedback(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawTooltip(juce::Graphics& g);
    juce::String formatTimePosition(double beat) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};

