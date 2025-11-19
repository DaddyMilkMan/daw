/**
 * @file ClipComponent.h
 * @brief Visual representation of a clip on the timeline
 */

#pragma once

#include <JuceHeader.h>

/**
 * @class ClipComponent
 * @brief Displays a clip on the arranger timeline
 *
 * Shows clip name, position, and allows mouse interaction.
 * Bound to a CLIP ValueTree node.
 */
class ClipComponent : public juce::Component
{
public:
    /**
     * @brief Constructor
     * @param clipNode ValueTree node for this clip
     */
    ClipComponent(juce::ValueTree clipNode);
    ~ClipComponent() override = default;

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
    void updateBounds(double pixelsPerBeat, int yPosition, int height);

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    juce::ValueTree clip;
    juce::Point<int> dragStartPos;
    double dragStartBeats = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
