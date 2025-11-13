/**
 * @file ArrangerComponent.h
 * @brief Minimal timeline/arranger view for v0.1
 *
 * Displays tracks and clips on a horizontal time axis.
 * Supports:
 * - Visual representation of tracks and clips
 * - Click to set playhead
 * - Drag clips horizontally to change startSample
 * - Playhead animation during playback
 *
 * v0.1 Constraints (intentionally minimal):
 * - No zoom/scroll
 * - No vertical drag (no moving clips between tracks)
 * - No resizing clips
 * - No snapping/grid
 * - No selection, multi-select, delete
 * - No automation, MIDI, tempo
 */

#pragma once

#include <JuceHeader.h>
#include "../../include/editor/ProjectEditorState.h"
#include "../../include/model/ProjectModel.h"

/**
 * @class ArrangerComponent
 * @brief Minimal timeline view for arranging audio clips
 *
 * Coordinate System:
 * - Horizontal: seconds → pixels (pixelsPerSecond = 100)
 * - Vertical: fixed track height (60px per track)
 * - Origin: top-left of timeline area (after headers)
 */
class ArrangerComponent : public juce::Component,
                         public juce::Timer
{
public:
    explicit ArrangerComponent(zenith::ProjectEditorState& editorState);
    ~ArrangerComponent() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse interactions
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    //==========================================================================
    // Timer: poll playhead position while playing
    //==========================================================================

    void timerCallback() override;

private:
    //==========================================================================
    // Drag State
    //==========================================================================

    struct DragState
    {
        bool active = false;
        int trackIndex = -1;
        juce::int64 originalClipStart = 0;  // in samples
        juce::int64 dragStartSample = 0;    // timeline sample at mouseDown
        juce::int64 clipId = -1;
    };

    DragState drag_;

    //==========================================================================
    // Layout Constants
    //==========================================================================

    float pixelsPerSecond_ = 100.0f;  // v0.1: fixed zoom
    int trackHeaderWidth_ = 120;
    int trackHeight_ = 60;
    int timelineHeight_ = 30;  // space for time ruler at top

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Get bounds for a track's content area (clip lane)
     */
    juce::Rectangle<int> getTrackBounds(int trackIndex) const;

    /**
     * @brief Get bounds for a specific clip rectangle
     */
    juce::Rectangle<int> getClipBounds(const zenith::TrackModel& track,
                                       const zenith::ClipModel& clip,
                                       int trackIndex) const;

    /**
     * @brief Get project sample rate
     */
    double getProjectSampleRate() const;

    /**
     * @brief Convert samples to x pixel coordinate
     */
    double samplesToX(juce::int64 samples) const;

    /**
     * @brief Convert x pixel coordinate to samples
     */
    juce::int64 xToSamples(int x) const;

    /**
     * @brief Find clip at mouse position (returns trackIndex and clipId)
     * @return true if clip found, sets outTrackIndex and outClipId
     */
    bool findClipAtPosition(juce::Point<int> pos, int& outTrackIndex, juce::int64& outClipId);

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawBackground(juce::Graphics& g);
    void drawTracksAndClips(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);

    //==========================================================================
    // Member Variables
    //==========================================================================

    zenith::ProjectEditorState& editor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
