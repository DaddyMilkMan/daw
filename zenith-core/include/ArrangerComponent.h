/**
 * @file ArrangerComponent.h
 * @brief Timeline/Arranger view with clip editing
 *
 * Phase 9: Arranger MVP
 * - Visual timeline showing tracks and clips
 * - Clip selection (single + multi-select)
 * - Clip editing: move, resize, create, delete, duplicate
 * - Zoom and scroll
 * - All operations use ProjectState + UndoManager
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @struct ClipView
 * @brief Lightweight UI representation of a clip
 *
 * This struct maps ProjectState clip data to screen coordinates for
 * drawing and hit-testing. The source of truth is always ProjectState.
 */
struct ClipView
{
    juce::String clipId;
    juce::String trackId;

    double startBeats{0.0};
    double lengthBeats{1.0};
    bool isMidi{false};
    bool isSelected{false};

    juce::Rectangle<float> bounds; // Screen coordinates for drawing/hit-testing

    // Helper to check if a point is in the left resize zone
    bool isInLeftResizeZone(juce::Point<float> point, float zoneWidth = 6.0f) const
    {
        return bounds.contains(point) && point.x < bounds.getX() + zoneWidth;
    }

    // Helper to check if a point is in the right resize zone
    bool isInRightResizeZone(juce::Point<float> point, float zoneWidth = 6.0f) const
    {
        return bounds.contains(point) && point.x > bounds.getRight() - zoneWidth;
    }
};

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Timeline view with interactive clip editing
 *
 * Features:
 * - Displays tracks as horizontal lanes
 * - Shows clips as colored rectangles on timeline
 * - Single + multi-select clips
 * - Move clips in time and between tracks
 * - Resize clips from edges
 * - Create clips by double-clicking
 * - Delete and duplicate clips
 * - Zoom and scroll timeline
 *
 * All editing operations go through ProjectState and use UndoManager.
 * This component is message-thread only (no RT audio thread interaction).
 */
class ArrangerComponent : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    explicit ArrangerComponent(ProjectState& projectState);
    ~ArrangerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // Drag modes
    //==========================================================================

    enum class DragMode
    {
        None,
        MoveClips,
        ResizeClipLeft,
        ResizeClipRight,
        Marquee
    };

    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;

    //==========================================================================
    // Clip view management
    //==========================================================================

    /**
     * @brief Rebuild clip views from ProjectState
     */
    void rebuildClipViews();

    /**
     * @brief Recompute screen bounds for all clip views
     */
    void recomputeClipBounds();

    /**
     * @brief Find clip view by ID
     */
    ClipView* findClipView(const juce::String& clipId);

    /**
     * @brief Hit-test to find clip at point
     */
    ClipView* findClipAtPoint(juce::Point<float> point);

    //==========================================================================
    // Coordinate conversion
    //==========================================================================

    float beatsToX(double beats) const;
    double xToBeats(float x) const;
    float trackIndexToY(int trackIndex) const;
    int yToTrackIndex(float y) const;

    /**
     * @brief Snap beats to grid
     */
    double snapToGrid(double beats) const;

    //==========================================================================
    // Selection management
    //==========================================================================

    void clearSelection();
    void selectClip(const juce::String& clipId, bool addToSelection) [[maybe_unused]];
    void selectClipsInRect(juce::Rectangle<float> rect);
    bool isClipSelected(const juce::String& clipId) const;

    //==========================================================================
    // Clip operations (via ProjectState)
    //==========================================================================

    void createClipAtPoint(juce::Point<float> point);
    void deleteSelectedClips();
    void duplicateSelectedClips();

    //==========================================================================
    // Painting helpers
    //==========================================================================

    void paintBackground(juce::Graphics& g);
    void paintTimeRuler(juce::Graphics& g);
    void paintTracks(juce::Graphics& g);
    void paintClips(juce::Graphics& g);
    void paintMarquee(juce::Graphics& g);

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;

    // Clip views (UI model)
    juce::Array<ClipView> clipViews;

    // Selection
    juce::SortedSet<juce::String> selectedClipIds;

    // Timeline state
    double pixelsPerBeat{40.0};
    double viewStartBeats{0.0};
    int firstVisibleTrackIndex{0};
    int trackHeight{80};
    int rulerHeight{30};

    // Grid snap
    double gridSnapBeats{0.25}; // 1/16 note at 4/4

    // Drag state
    DragMode currentDragMode{DragMode::None};
    juce::Point<float> dragStartPoint;
    juce::Rectangle<float> marqueeRect;

    // For moving clips
    struct ClipDragState
    {
        juce::String clipId;
        double originalStartBeats;
        int originalTrackIndex;
    };
    juce::Array<ClipDragState> clipDragStates;

    // For resizing clips
    juce::String resizingClipId;
    double resizeOriginalStart;
    double resizeOriginalLength;

    // Colors
    juce::Colour audioClipColour{0xff4a90e2};     // Blue
    juce::Colour midiClipColour{0xff7ed321};      // Green
    juce::Colour selectedClipColour{0xffffffff};  // White border
    juce::Colour trackLaneColour{0xff2a2a2a};     // Dark grey
    juce::Colour trackDividerColour{0xff1a1a1a};  // Darker grey
    juce::Colour gridLineColour{0xff333333};      // Medium grey

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

