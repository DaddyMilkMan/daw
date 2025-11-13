/**
 * @file ArrangerComponent.h
 * @brief Timeline/arranger view with zoom and scroll
 *
 * Displays tracks and clips on a horizontal time axis.
 * Supports:
 * - Visual representation of tracks and clips
 * - Zoom (Ctrl/Cmd + wheel) and scroll (Shift + wheel)
 * - Time ruler with second marks
 * - Click to set playhead
 * - Drag clips horizontally to change startSample
 * - Playhead animation during playback
 *
 * v0.1.1 Constraints (intentionally minimal):
 * - No vertical drag (no moving clips between tracks)
 * - No resizing clips
 * - No snapping (basic snap coming soon)
 * - No selection, multi-select, delete
 * - No automation, MIDI, tempo
 */

#pragma once

#include <JuceHeader.h>
#include "../../include/editor/ProjectEditorState.h"
#include "../../include/model/ProjectModel.h"

/**
 * @class ArrangerComponent
 * @brief Timeline view for arranging audio clips with zoom and scroll
 *
 * Coordinate System:
 * - Horizontal: samples → pixels (dynamic zoom via samplesPerPixel_)
 * - Vertical: fixed track height (60px per track)
 * - Origin: top-left of timeline area (after headers)
 * - Scroll offset: scrollOffsetSamples_ defines left edge of view
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
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

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

    int trackHeaderWidth_ = 120;
    int trackHeight_ = 60;
    int rulerHeight_ = 20;  // space for time ruler at top

    //==========================================================================
    // Zoom & Scroll State
    //==========================================================================

    double samplesPerPixel_ = 4800.0;       // default: 100 px/sec at 48kHz
    double minSamplesPerPixel_ = 480.0;     // max zoom-in  (10x closer)
    double maxSamplesPerPixel_ = 48000.0;   // max zoom-out (10x further)
    juce::int64 scrollOffsetSamples_ = 0;   // left edge of view in samples

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
     * @brief Convert samples to x pixel coordinate (includes scroll offset)
     */
    int samplesToX(juce::int64 samples) const;

    /**
     * @brief Convert x pixel coordinate to samples (includes scroll offset)
     */
    juce::int64 xToSamples(int x) const;

    /**
     * @brief Get visible sample range for current view
     * @return [start, end) in samples based on scroll offset and component width
     */
    juce::Range<juce::int64> getVisibleSampleRange() const;

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
