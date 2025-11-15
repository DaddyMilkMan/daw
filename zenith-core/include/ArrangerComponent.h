/**
 * @file ArrangerComponent.h
 * @brief Main timeline/arranger view with tempo map and markers
 *
 * Phase 15: Tempo Map + Markers v1
 *
 * Displays:
 * - Tempo lane (top) showing tempo curve and tempo points
 * - Marker lane showing markers with labels
 * - Timeline/arranger area (future: tracks and clips)
 *
 * Interactions:
 * - Add tempo points (double-click tempo lane)
 * - Move tempo points (drag)
 * - Delete tempo points (select + Delete key)
 * - Add markers (double-click marker lane)
 * - Move markers (drag)
 * - Rename markers (double-click label)
 * - Delete markers (select + Delete key)
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"

/**
 * @class ArrangerComponent
 * @brief Timeline view with tempo map and markers
 *
 * Thread safety:
 * - All UI operations run on message thread
 * - Listens to ValueTree changes for undo/redo support
 * - Calls ProjectState APIs which handle undo automatically
 */
class ArrangerComponent : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    ArrangerComponent(ProjectState& projectState, Engine& engine);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // Layout Constants
    //==========================================================================

    static constexpr int TEMPO_LANE_HEIGHT = 80;
    static constexpr int MARKER_LANE_HEIGHT = 30;
    static constexpr int TIME_RULER_HEIGHT = 20;

    // Tempo range for display
    static constexpr double MIN_TEMPO = 40.0;
    static constexpr double MAX_TEMPO = 240.0;

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================

    /**
     * @brief Convert beat position to x coordinate
     */
    float beatsToX(double beats) const;

    /**
     * @brief Convert x coordinate to beat position
     */
    double xToBeats(float x) const;

    /**
     * @brief Convert BPM to y coordinate in tempo lane
     */
    float bpmToY(double bpm) const;

    /**
     * @brief Convert y coordinate in tempo lane to BPM
     */
    double yToBpm(float y) const;

    /**
     * @brief Snap beats to grid (1/4 beat for now)
     */
    double snapBeats(double beats) const;

    //==========================================================================
    // Tempo Lane Rendering
    //==========================================================================

    void paintTempoLane(juce::Graphics& g, juce::Rectangle<int> area);
    void paintTempoCurve(juce::Graphics& g, juce::Rectangle<int> area);
    void paintTempoPoints(juce::Graphics& g, juce::Rectangle<int> area);

    //==========================================================================
    // Marker Lane Rendering
    //==========================================================================

    void paintMarkerLane(juce::Graphics& g, juce::Rectangle<int> area);
    void paintMarkers(juce::Graphics& g, juce::Rectangle<int> area);

    //==========================================================================
    // Time Ruler Rendering
    //==========================================================================

    void paintTimeRuler(juce::Graphics& g, juce::Rectangle<int> area);

    //==========================================================================
    // Interaction Helpers
    //==========================================================================

    /**
     * @brief Find tempo point near position (or empty string if none)
     */
    juce::String findTempoPointNear(float x, float y, float tolerance = 8.0f);

    /**
     * @brief Find marker near position (or empty string if none)
     */
    juce::String findMarkerNear(float x, float y, float tolerance = 8.0f);

    /**
     * @brief Check if point is in tempo lane
     */
    bool isInTempoLane(float y) const;

    /**
     * @brief Check if point is in marker lane
     */
    bool isInMarkerLane(float y) const;

    /**
     * @brief Show marker rename dialog
     */
    void showMarkerRenameDialog(const juce::String& markerId);

    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState_;
    Engine& engine_;

    // View state
    double pixelsPerBeat_{40.0};  // Zoom level
    double viewOffsetBeats_{0.0}; // Horizontal scroll

    // Interaction state
    enum class DragMode
    {
        None,
        TempoPoint,
        Marker
    };

    DragMode dragMode_{DragMode::None};
    juce::String draggedItemId_;
    juce::Point<float> dragStartPos_;
    double dragStartBeats_{0.0};
    double dragStartBpm_{120.0};

    // Selection state
    juce::String selectedTempoPointId_;
    juce::String selectedMarkerId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
