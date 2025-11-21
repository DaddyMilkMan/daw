/**
 * @file ClipComponent.h
 * @brief Beautiful clip with gradients, animations, and modern design
 *
 * Features modern DAW aesthetics:
 * - Smooth gradients (Ableton-inspired)
 * - Hover effects with scale animation
 * - Selection glow with pulse
 * - Subtle shadows for depth
 * - Waveform preview visualization
 * - 60 Hz smooth animations
 */

#pragma once

#include <JuceHeader.h>

/**
 * @class ClipComponent
 * @brief Beautiful, animated clip display on the arranger timeline
 *
 * Modern design with:
 * - Gradient backgrounds (lighter at top, darker at bottom)
 * - Hover scaling and glow effects
 * - Selection pulse animation
 * - Shadows for depth
 * - Rounded corners (8px)
 * - Waveform preview for audio clips
 */
class ClipComponent : public juce::Component,
                     public juce::Timer
{
public:
    /**
     * @brief Constructor
     * @param clipNode ValueTree node for this clip
     */
    ClipComponent(juce::ValueTree clipNode);
    ~ClipComponent() override;

    //==========================================================================
    // Clip data
    //==========================================================================

    /**
     * @brief Get the clip's ValueTree node
     */
    juce::ValueTree getClipNode() const { return clip; }

    /**
     * @brief Get clip ID
     */
    juce::String getClipId() const;

    /**
     * @brief Get start position in beats
     */
    double getStartBeats() const;

    /**
     * @brief Get length in beats
     */
    double getLengthBeats() const;

    /**
     * @brief Update bounds from clip data and pixels-per-beat ratio
     */
    void updateBounds(double pixelsPerBeat, int yPosition, int height) [[maybe_unused]];

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    //==========================================================================
    // Timer interface (for smooth animations)
    //==========================================================================

    void timerCallback() override;

private:
    juce::ValueTree clip;
    juce::Point<int> dragStartPos;
    double dragStartBeats = 0.0;

    // Animation state
    bool isHovered = false;
    bool isSelected = false;
    float hoverAnimation = 0.0f;     // 0.0 to 1.0 for smooth hover animation
    float selectionPulse = 0.0f;     // 0.0 to 1.0 for selection glow pulse

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};

