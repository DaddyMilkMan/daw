/**
 * @file ArrangementComponent.h
 * @brief Arrangement view with automation lanes (Phase 14)
 *
 * Displays:
 * - Tracks with headers
 * - Clips on timeline
 * - Automation lanes (volume/pan/mute) per track
 *
 * Features:
 * - Per-track automation lane selection (V/P/M buttons)
 * - Visual automation curve display
 * - Add/move/delete automation points with mouse
 * - Full undo/redo support via ProjectState
 * - Live updates from CommandAPI/Wingman
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

class ProjectState;
class Engine;

//==============================================================================
/**
 * @struct TrackUIState
 * @brief Per-track UI state (not stored in ProjectState)
 *
 * This is ephemeral UI state that doesn't need to be saved.
 */
struct TrackUIState
{
    juce::String trackId;
    juce::String visibleAutomationParam;  // "volume", "pan", "mute", or empty
    int trackIndex = -1;
};

//==============================================================================
/**
 * @struct AutomationPointSelection
 * @brief Currently selected automation point
 */
struct AutomationPointSelection
{
    juce::String trackId;
    juce::String paramId;
    juce::String pointId;
    double originalTimeBeats = 0.0;
    double originalValue = 0.0;
    bool isDragging = false;

    void clear()
    {
        trackId = "";
        paramId = "";
        pointId = "";
        isDragging = false;
    }

    bool isValid() const { return !pointId.isEmpty(); }
};

//==============================================================================
/**
 * @struct AutomationPointView
 * @brief Lightweight view of an automation point for rendering
 */
struct AutomationPointView
{
    juce::String pointId;
    double timeBeats;
    double value;
};

//==============================================================================
/**
 * @class ArrangementComponent
 * @brief Main timeline/arrangement view with automation support
 *
 * Layout:
 * - Left: Track headers (names, automation lane buttons)
 * - Right: Timeline view (clips + automation lanes)
 *
 * Thread safety:
 * - All methods run on MESSAGE THREAD only
 * - Reads from ProjectState ValueTree (message thread safe)
 * - Modifies ProjectState via undoable API calls
 */
class ArrangementComponent : public juce::Component,
                              public juce::ValueTree::Listener,
                              private juce::Timer
{
public:
    //==========================================================================
    ArrangementComponent(ProjectState& projectState, Engine& engine);
    ~ArrangementComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse interaction
    //==========================================================================

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    //==========================================================================
    // Keyboard interaction
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key) override;

    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

private:
    //==========================================================================
    // Timer callback (for UI updates)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Drawing helpers
    //==========================================================================

    void drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTimeline(juce::Graphics& g, juce::Rectangle<int> area);
    void drawTrack(juce::Graphics& g, int trackIndex, juce::Rectangle<int> area);
    void drawAutomationLane(juce::Graphics& g, const juce::String& trackId,
                           const juce::String& paramId, juce::Rectangle<int> area);

    //==========================================================================
    // Coordinate mapping
    //==========================================================================

    double pixelsToBeats(int pixels) const;
    int beatsToPixels(double beats) const;
    double pixelsToAutomationValue(int y, juce::Rectangle<int> laneArea, const juce::String& paramId) const;
    int automationValueToPixels(double value, juce::Rectangle<int> laneArea, const juce::String& paramId) const;

    //==========================================================================
    // Hit testing
    //==========================================================================

    bool isPointInAutomationLane(juce::Point<int> pos, int trackIndex) const;
    juce::String findAutomationPointAtPosition(juce::Point<int> pos, int trackIndex,
                                               juce::String& outParamId, juce::String& outPointId);
    juce::Rectangle<int> getTrackArea(int trackIndex) const;
    juce::Rectangle<int> getAutomationLaneArea(int trackIndex) const;
    juce::Rectangle<int> getTrackHeaderArea(int trackIndex) const;

    //==========================================================================
    // Automation editing
    //==========================================================================

    void addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                           double timeBeats, double value);
    void startDraggingPoint(const juce::String& trackId, const juce::String& paramId,
                           const juce::String& pointId, double timeBeats, double value);
    void updateDraggingPoint(double newTimeBeats, double newValue) [[maybe_unused]];
    void finishDraggingPoint();
    void deleteSelectedPoint();

    //==========================================================================
    // Track UI state management
    //==========================================================================

    void rebuildTrackUIState();
    void setVisibleAutomationParam(int trackIndex, const juce::String& paramId);
    juce::String getVisibleAutomationParam(int trackIndex) const;

    //==========================================================================
    // Helpers
    //==========================================================================

    juce::Array<AutomationPointView> getAutomationPoints(const juce::String& trackId,
                                                          const juce::String& paramId) const;
    juce::Colour getAutomationColour(const juce::String& paramId) const;
    double snapToGrid(double beats) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    // UI state
    std::vector<TrackUIState> trackUIStates;
    AutomationPointSelection selectedPoint;

    // Layout constants
    static constexpr int TRACK_HEIGHT = 100;
    static constexpr int TRACK_HEADER_WIDTH = 200;
    static constexpr int AUTOMATION_LANE_HEIGHT_RATIO = 30; // 30% of track height
    static constexpr int GRID_SNAP_BEATS = 1; // Snap to 1/16 beats (0.25)

    // View state
    double pixelsPerBeat = 50.0;
    double viewOffsetBeats = 0.0;
    int lastTrackCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementComponent)
};

