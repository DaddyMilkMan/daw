/**
 * @file ClipComponent.h
 * @brief Visual representation of a clip with drag support
 *
 * Arranger Timeline UI - Phase 14
 * Displays a single clip and allows dragging to change position
 */

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * @class ClipComponent
 * @brief Visual representation of a clip
 *
 * Features:
 * - Visual rectangle showing clip bounds
 * - Drag-to-move with beat snapping
 * - Displays clip name/ID
 * - Mouse hover feedback
 */
class ClipComponent : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Callback for when clip is moved
     * @param clipId Clip ID
     * @param newStartBeats New start position in beats
     */
    std::function<void(const juce::String&, double)> onClipMoved;

    //==========================================================================
    ClipComponent(const juce::String& id, const juce::String& trackId,
                  double startBeats, double lengthBeats);
    ~ClipComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==========================================================================
    // Clip properties
    //==========================================================================

    juce::String getClipId() const { return clipId; }
    juce::String getTrackId() const { return trackId; }
    double getStartBeats() const { return startBeats; }
    double getLengthBeats() const { return lengthBeats; }

    void setStartBeats(double newStart);
    void setLengthBeats(double newLength);

    //==========================================================================
    // Grid snapping
    //==========================================================================

    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    void setSnapGrid(double beatsPerSnap) { snapGridBeats = beatsPerSnap; }

private:
    //==========================================================================
    // Helper methods
    //==========================================================================

    double snapToGrid(double beats) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    juce::String clipId;
    juce::String trackId;
    double startBeats;
    double lengthBeats;

    // Dragging state
    bool isDragging = false;
    bool isMouseOver = false;
    double dragStartBeats = 0.0;
    int dragStartX = 0;

    // Snapping
    bool snapEnabled = true;
    double snapGridBeats = 0.25;  // 1/16th note by default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
