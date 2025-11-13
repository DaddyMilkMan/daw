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
    // Keyboard interactions
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key) override;

    //==========================================================================
    // Timer: poll playhead position while playing
    //==========================================================================

    void timerCallback() override;

private:
    //==========================================================================
    // Selection State
    //==========================================================================

    struct SelectedClip
    {
        int trackIndex = -1;
        int clipIndex = -1;   // index into TrackModel::clips
        bool isValid() const { return trackIndex >= 0 && clipIndex >= 0; }
    };

    SelectedClip selected_;
    int selectedTrackIndex_ = -1;  // Selected track (-1 = none)

    //==========================================================================
    // Drag State
    //==========================================================================

    enum class DragMode
    {
        None,
        Move,
        TrimLeft,
        TrimRight,
        AdjustGain,
        AdjustPan
    };

    DragMode dragMode_ = DragMode::None;
    SelectedClip dragClip_;  // The clip being manipulated
    juce::int64 dragOriginalStartSamples_ = 0;
    juce::int64 dragOriginalEndSamples_ = 0;  // start + resolved length
    int dragTrackForMixer_ = -1;  // Track index for gain/pan drag

    //==========================================================================
    // Layout Constants
    //==========================================================================

    static constexpr int kTrackHeaderWidth = 180;
    int trackHeight_ = 60;
    int rulerHeight_ = 20;  // space for time ruler at top

    static constexpr float kEdgeHotZonePixels = 6.0f;  // edge detection zone

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
     * @brief Hit test result with clip and drag mode
     */
    struct HitTestResult
    {
        SelectedClip clip;
        DragMode mode = DragMode::None;
    };

    /**
     * @brief Hit-test for clip at mouse position with edge detection
     * @return HitTestResult with clip and drag mode
     */
    HitTestResult hitTestClip(juce::Point<float> pos) const;

    /**
     * @brief Hit-test for track header at mouse position
     * @param pos Mouse position
     * @return Track index or -1 if not in header area
     */
    int hitTestTrackHeader(juce::Point<int> pos) const;

    /**
     * @brief Hit-test for mute button in track header
     * @param pos Mouse position
     * @param outTrackIndex Output track index if hit
     * @return true if mouse is over a mute button
     */
    bool hitTestMuteButton(juce::Point<int> pos, int& outTrackIndex) const;

    /**
     * @brief Hit-test for solo button in track header
     * @param pos Mouse position
     * @param outTrackIndex Output track index if hit
     * @return true if mouse is over a solo button
     */
    bool hitTestSoloButton(juce::Point<int> pos, int& outTrackIndex) const;

    /**
     * @brief Hit-test for gain slider in track header
     * @param pos Mouse position
     * @param outTrackIndex Output track index if hit
     * @return true if mouse is over a gain slider
     */
    bool hitTestGainSlider(juce::Point<int> pos, int& outTrackIndex) const;

    /**
     * @brief Hit-test for pan slider in track header
     * @param pos Mouse position
     * @param outTrackIndex Output track index if hit
     * @return true if mouse is over a pan slider
     */
    bool hitTestPanSlider(juce::Point<int> pos, int& outTrackIndex) const;

    /**
     * @brief Get bounds for track header rectangle
     * @param trackIndex Track index
     * @return Rectangle for track header
     */
    juce::Rectangle<int> getTrackHeaderBounds(int trackIndex) const;

    /**
     * @brief Get bounds for mute button within a track header
     * @param trackIndex Track index
     * @return Rectangle for mute button
     */
    juce::Rectangle<int> getMuteButtonBounds(int trackIndex) const;

    /**
     * @brief Get bounds for solo button within a track header
     * @param trackIndex Track index
     * @return Rectangle for solo button
     */
    juce::Rectangle<int> getSoloButtonBounds(int trackIndex) const;

    /**
     * @brief Get bounds for gain slider within a track header
     * @param trackIndex Track index
     * @return Rectangle for gain slider
     */
    juce::Rectangle<int> getGainSliderBounds(int trackIndex) const;

    /**
     * @brief Get bounds for pan slider within a track header
     * @param trackIndex Track index
     * @return Rectangle for pan slider
     */
    juce::Rectangle<int> getPanSliderBounds(int trackIndex) const;

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawBackground(juce::Graphics& g);
    void drawTrackHeaders(juce::Graphics& g);
    void drawTracksAndClips(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);

    //==========================================================================
    // Member Variables
    //==========================================================================

    zenith::ProjectEditorState& editor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};
