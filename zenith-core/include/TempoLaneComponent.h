/**
 * @file TempoLaneComponent.h
 * @brief UI component for displaying and editing tempo map
 *
 * Phase 15: Tempo Map & Global Markers MVP
 *
 * Displays tempo points as nodes along a timeline.
 * Allows adding, moving, and deleting tempo points via mouse interaction.
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class TempoLaneComponent
 * @brief UI lane for tempo map editing
 *
 * User interactions:
 * - Double-click to add tempo point
 * - Drag to move tempo point (horizontal = time, vertical = BPM)
 * - Select point + Delete/Backspace to remove
 */
class TempoLaneComponent : public juce::Component,
                           private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     */
    explicit TempoLaneComponent(ProjectState& projectState);

    /**
     * @brief Destructor
     */
    ~TempoLaneComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Convert X coordinate to beats
     */
    double xToBeats(float x) const;

    /**
     * @brief Convert beats to X coordinate
     */
    float beatsToX(double beats) const;

    /**
     * @brief Convert Y coordinate to BPM
     */
    double yToBpm(float y) const;

    /**
     * @brief Convert BPM to Y coordinate
     */
    float bpmToY(double bpm) const;

    /**
     * @brief Find tempo point at position (returns ID or empty string)
     */
    juce::String findPointAt(float x, float y) const;

    /**
     * @brief Draw a tempo point
     */
    void drawTempoPoint(juce::Graphics& g, double timeBeats, double bpm, bool selected) [[maybe_unused]];

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;

    // View settings (MVP: fixed range, no zoom/scroll)
    double viewStartBeats = 0.0;
    double viewEndBeats = 64.0;  // 16 bars at 4/4
    double minBpm = 40.0;
    double maxBpm = 240.0;

    // Interaction state
    juce::String selectedPointId;
    juce::String draggingPointId;
    juce::Point<float> dragStart;
    double dragStartBeats = 0.0;
    double dragStartBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoLaneComponent)
};

