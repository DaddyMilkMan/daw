/**
 * @file ArrangerView.h
 * @brief Main arranger timeline view
 *
 * Arranger Timeline UI - Phase 14
 * Beat-based timeline with one lane per track, showing clips
 */

#pragma once

#include <JuceHeader.h>
#include "../ProjectState.h"
#include "TimelineRuler.h"
#include "ClipComponent.h"

//==============================================================================
/**
 * @class TrackLane
 * @brief Represents a single track lane in the arranger
 */
class TrackLane : public juce::Component,
                  private juce::ValueTree::Listener
{
public:
    TrackLane(ProjectState& state, const juce::String& trackId);
    ~TrackLane() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::String getTrackId() const { return trackId; }
    void setPixelsPerBeat(double ppb);
    void setScrollOffset(int offset);

    void refreshClips();

private:
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    void onClipMoved(const juce::String& clipId, double newStartBeats);

    ProjectState& projectState;
    juce::String trackId;
    juce::String trackName;
    double pixelsPerBeat = 40.0;
    int scrollOffset = 0;

    juce::OwnedArray<ClipComponent> clipComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLane)
};

//==============================================================================
/**
 * @class ArrangerView
 * @brief Main arranger view with timeline and track lanes
 *
 * Features:
 * - TimelineRuler at top
 * - One TrackLane per track
 * - Horizontal scrolling
 * - Zoom (pixels per beat)
 * - Listens to ProjectState for changes
 */
class ArrangerView : public juce::Component,
                     private juce::ValueTree::Listener,
                     private juce::ScrollBar::Listener
{
public:
    //==========================================================================
    ArrangerView(ProjectState& state);
    ~ArrangerView() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Zoom control
    //==========================================================================

    void setPixelsPerBeat(double ppb);
    double getPixelsPerBeat() const { return pixelsPerBeat; }

    void zoomIn();
    void zoomOut();

private:
    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;

    //==========================================================================
    // ScrollBar::Listener interface
    //==========================================================================

    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;

    //==========================================================================
    // Helper methods
    //==========================================================================

    void refreshTracks();
    void updateScrollBars();

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;

    // UI Components
    std::unique_ptr<TimelineRuler> timelineRuler;
    juce::OwnedArray<TrackLane> trackLanes;

    // Scrollbars
    juce::ScrollBar horizontalScrollBar;
    juce::ScrollBar verticalScrollBar;

    // View state
    double pixelsPerBeat = 40.0;
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;

    static constexpr int RULER_HEIGHT = 40;
    static constexpr int TRACK_HEIGHT = 80;
    static constexpr int SCROLLBAR_SIZE = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};
