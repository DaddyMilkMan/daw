/**
 * @file ArrangerComponent.h
 * @brief Arranger/Timeline component with automation lane editing
 *
 * Phase 14: Arranger Automation Lanes UI
 * - Visual representation of tracks as horizontal lanes
 * - Automation lane editing for volume, pan, and mute
 * - V/P/M toggle buttons per track
 * - Mouse-based automation point editing
 * - Full undo/redo integration via ProjectState
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @struct TrackAutomationUIState
 * @brief UI-only state for track automation lane visibility
 *
 * This is view state only - not stored in ProjectState
 */
struct TrackAutomationUIState
{
    juce::String trackId;
    bool showVolumeLane = false;
    bool showPanLane = false;
    bool showMuteLane = false;
};

//==============================================================================
/**
 * @struct AutomationPointView
 * @brief View representation of a single automation point
 */
struct AutomationPointView
{
    juce::String id;
    double timeBeats;
    double value;

    AutomationPointView() : timeBeats(0.0), value(0.0) {}
    AutomationPointView(const juce::String& pointId, double t, double v)
        : id(pointId), timeBeats(t), value(v) {}
};

//==============================================================================
/**
 * @struct AutomationCurveView
 * @brief Cached view of an automation curve for efficient rendering
 */
struct AutomationCurveView
{
    juce::String trackId;
    juce::String param;
    std::vector<AutomationPointView> points;
    bool needsRebuild = true;

    AutomationCurveView() = default;
    AutomationCurveView(const juce::String& tid, const juce::String& p)
        : trackId(tid), param(p) {}
};

//==============================================================================
/**
 * @struct SelectedAutomationPoint
 * @brief Currently selected automation point (single selection in v1)
 */
struct SelectedAutomationPoint
{
    juce::String trackId;
    juce::String param;
    juce::String pointId;
    bool isValid = false;

    void clear()
    {
        trackId = "";
        param = "";
        pointId = "";
        isValid = false;
    }

    void set(const juce::String& tid, const juce::String& p, const juce::String& pid)
    {
        trackId = tid;
        param = p;
        pointId = pid;
        isValid = true;
    }
};

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Main timeline/arranger view with automation lane editing
 *
 * Features:
 * - Shows tracks as horizontal lanes
 * - Per-track automation lanes for volume, pan, mute
 * - V/P/M toggle buttons in track headers
 * - Mouse-based editing: add, move, delete automation points
 * - ValueTree listeners for automatic UI updates
 * - Full undo/redo integration
 *
 * Layout:
 * - Track header (left, 150px): Track name + V/P/M buttons
 * - Timeline area (right): Beat-based grid with automation lanes
 * - Track height: 80px base + 60px per visible automation lane
 */
class ArrangerComponent : public juce::Component,
                          private juce::ValueTree::Listener,
                          private juce::KeyListener
{
public:
    //==========================================================================
    ArrangerComponent(ProjectState& projectState);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    //==========================================================================
    // Coordinate conversion
    //==========================================================================

    /** Convert beats to X coordinate */
    float beatToX(double beats) const;

    /** Convert X coordinate to beats */
    double xToBeat(float x) const;

    /** Convert parameter value to Y coordinate within a lane */
    float valueToY(double value, const juce::String& param, const juce::Rectangle<float>& laneArea) const;

    /** Convert Y coordinate to parameter value within a lane */
    double yToValue(float y, const juce::String& param, const juce::Rectangle<float>& laneArea) const;

    /** Snap time to grid (1/16 beat increments) */
    double snapToGrid(double beats) const;

private:
    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int indexFromWhichChildWasRemoved) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // KeyListener interface
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    //==========================================================================
    // Track lane management
    //==========================================================================

    /** Get track lane bounds (entire track row) */
    juce::Rectangle<int> getTrackLaneBounds(int trackIndex) const;

    /** Get automation lane bounds for a specific parameter */
    juce::Rectangle<float> getAutomationLaneBounds(int trackIndex, const juce::String& param) const;

    /** Get track index at Y position */
    int getTrackIndexAtY(int y) const;

    /** Get track height including visible automation lanes */
    int getTrackHeight(int trackIndex) const;

    /** Toggle automation lane visibility */
    void toggleAutomationLane(const juce::String& trackId, const juce::String& param);

    /** Find or create UI state for track */
    TrackAutomationUIState& getOrCreateUIState(const juce::String& trackId);

    //==========================================================================
    // Rendering
    //==========================================================================

    /** Paint track header (left side) */
    void paintTrackHeader(juce::Graphics& g, int trackIndex, const juce::Rectangle<int>& area);

    /** Paint automation lane */
    void paintAutomationLane(juce::Graphics& g, int trackIndex, const juce::String& param,
                              const juce::Rectangle<float>& laneArea);

    /** Paint automation curve */
    void paintAutomationCurve(juce::Graphics& g, const AutomationCurveView& curve,
                               const juce::Rectangle<float>& laneArea, const juce::String& param);

    /** Paint automation points */
    void paintAutomationPoints(juce::Graphics& g, const AutomationCurveView& curve,
                                const juce::Rectangle<float>& laneArea, const juce::String& param);

    //==========================================================================
    // Automation curve cache
    //==========================================================================

    /** Rebuild automation curve cache from ValueTree */
    void rebuildAutomationCurve(const juce::String& trackId, const juce::String& param);

    /** Get or create automation curve view */
    AutomationCurveView& getOrCreateCurveView(const juce::String& trackId, const juce::String& param);

    /** Find curve view */
    AutomationCurveView* findCurveView(const juce::String& trackId, const juce::String& param);

    /** Mark all curves as needing rebuild */
    void markAllCurvesDirty();

    //==========================================================================
    // Mouse interaction
    //==========================================================================

    /** Hit test for automation point at position */
    bool hitTestAutomationPoint(const juce::Point<float>& pos, juce::String& outTrackId,
                                 juce::String& outParam, juce::String& outPointId);

    /** Add automation point at mouse position */
    void addAutomationPointAt(const juce::Point<float>& pos, const juce::String& trackId, const juce::String& param);

    /** Start dragging a point */
    void startDraggingPoint(const juce::String& trackId, const juce::String& param, const juce::String& pointId);

    /** Update point position during drag */
    void updateDraggedPoint(const juce::Point<float>& pos);

    /** Finish dragging point */
    void finishDraggingPoint();

    /** Delete selected point */
    void deleteSelectedPoint();

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;

    // Layout constants
    static constexpr int TRACK_HEADER_WIDTH = 150;
    static constexpr int TRACK_BASE_HEIGHT = 80;
    static constexpr int AUTOMATION_LANE_HEIGHT = 60;
    static constexpr int TOGGLE_BUTTON_SIZE = 24;
    static constexpr float PIXELS_PER_BEAT = 50.0f;
    static constexpr float POINT_RADIUS = 5.0f;
    static constexpr float HIT_TEST_RADIUS = 8.0f;
    static constexpr double GRID_SNAP = 0.0625; // 1/16 beat

    // UI state per track
    std::vector<TrackAutomationUIState> trackUIStates;

    // Automation curve cache
    std::vector<AutomationCurveView> curveCache;

    // Selection
    SelectedAutomationPoint selectedPoint;

    // Drag state
    bool isDragging = false;
    juce::String dragTrackId;
    juce::String dragParam;
    juce::String dragPointId;
    double dragStartTimeBeats = 0.0;
    double dragStartValue = 0.0;
    juce::Point<float> dragStartPos;

    // Hover state
    juce::String hoverTrackId;
    juce::String hoverParam;
    juce::String hoverPointId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
