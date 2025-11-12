/**
 * @file TrackView.h
 * @brief Central arrangement/track view for Zenith DAW (W5: Virtualized)
 *
 * W5 Enhancements:
 * - Virtualized painting (only draws visible tracks/clips)
 * - Zero allocations in paint() (cached fonts, paths, strings)
 * - Proper coordinate system with visible range helpers
 * - Ctrl+Wheel zoom around cursor
 * - Data model for tracks/clips (dummy UI data)
 * - Debug overlay for JUCE_DEBUG
 *
 * Replaces React components:
 * - vexel-daw/src/renderer/components/ArrangementView.tsx
 * - vexel-daw/src/renderer/components/Timeline.tsx
 * - vexel-daw/src/renderer/components/TrackList.tsx
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

//==============================================================================
// W5: Lightweight data models (UI-only, dummy data for testing)
//==============================================================================

struct Clip
{
    int trackIndex = 0;
    int64_t startSamples = 0;
    int64_t lengthSamples = 44100;  // 1 second @ 44.1kHz
    juce::Colour colour = juce::Colours::blue;
    juce::String name = "Clip";
};

struct Track
{
    juce::String name = "Track";
    int lanes = 1;  // For future: MIDI lanes, automation lanes
};

//==============================================================================
/**
 * @class TrackView
 * @brief Virtualized arrangement view with timeline and tracks
 *
 * W5 Performance Optimizations:
 * - O(visible) painting: only iterates visible tracks/clips
 * - Cached fonts, paths, grid labels (zero paint() allocations)
 * - Throttled repaints (only on scroll/zoom/data change, no idle repaint)
 * - Visible range math to skip offscreen rendering
 */
class TrackView : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Callback when clip selected
     */
    std::function<void(int clipId)> onClipSelected;

    /**
     * @brief Callback when track selected
     */
    std::function<void(int trackIndex)> onTrackSelected;

    /**
     * @brief Callback when playhead position clicked
     */
    std::function<void(double position)> onPlayheadClicked;

    /**
     * @brief W6: Callback after paint completes (for performance monitoring)
     */
    std::function<void(double paintTimeMs)> onPaintComplete;

    //==========================================================================
    TrackView();
    ~TrackView() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
     * @brief Set playhead position (in seconds)
     */
    void setPlayheadPosition(double positionInSeconds);

    /**
     * @brief Set BPM (affects grid/ruler)
     */
    void setBPM(double bpm);

    /**
     * @brief Set horizontal zoom (pixels per second)
     */
    void setPixelsPerSecond(double zoom);

    /**
     * @brief Set horizontal scroll offset (pixels)
     */
    void setHorizontalOffset(double offset);

    /**
     * @brief Set vertical scroll offset (pixels)
     */
    void setVerticalOffset(double offset);

    /**
     * @brief Set loop region (in seconds)
     */
    void setLoopRegion(double startInSeconds, double endInSeconds);

    /**
     * @brief Inject test session data (W5: for stress testing)
     */
    void setSessionData(std::vector<Track> newTracks, std::vector<Clip> newClips);

    /**
     * @brief W6: Get last paint stats (zero allocations)
     * @param outVisibleTracks Number of tracks painted in last frame
     * @param outVisibleClips Number of clips painted in last frame
     */
    void getLastPaintStats(int& outVisibleTracks, int& outVisibleClips) const;

private:
    //==========================================================================
    // Drawing helpers (W5: virtualized, zero allocations)
    //==========================================================================

    void drawTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);

    #if JUCE_DEBUG
        void drawDebugOverlay(juce::Graphics& g);
    #endif

    //==========================================================================
    // Coordinate system helpers (W5: visible range optimization)
    //==========================================================================

    /**
     * @brief Convert time (seconds) to X coordinate
     */
    double timeToX(double seconds) const;

    /**
     * @brief Convert X coordinate to time (seconds)
     */
    double xToTime(double x) const;

    /**
     * @brief Get visible track index range (for virtualization)
     */
    juce::Range<int> visibleTrackIndexRange() const;

    /**
     * @brief Get visible time range in seconds (for virtualization)
     */
    juce::Range<double> visibleTimeSecondsRange() const;

    //==========================================================================
    // Member variables - Data models (W5)
    //==========================================================================

    std::vector<Track> tracks;
    std::vector<Clip> clips;

    //==========================================================================
    // Member variables - Playback state
    //==========================================================================

    double playheadSeconds = 0.0;
    double currentBPM = 120.0;
    double sampleRate = 44100.0;  // For sample<->time conversion

    //==========================================================================
    // Member variables - Loop region
    //==========================================================================

    bool loopEnabled = false;
    double loopStartSeconds = 0.0;
    double loopEndSeconds = 8.0;

    //==========================================================================
    // Member variables - Coordinate system (W5)
    //==========================================================================

    double pixelsPerSecond = 60.0;    // Horizontal zoom
    double timeOffsetPx = 0.0;        // Horizontal scroll offset
    double verticalOffsetPx = 0.0;    // Vertical scroll offset
    int trackHeight = 80;             // Track height in pixels

    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr int timelineHeight = 32;
    static constexpr int trackHeaderWidth = 150;

    //==========================================================================
    // W5: Cached resources (zero allocations in paint)
    //==========================================================================

    juce::Font rulerFont {11.0f};              // Timeline ruler font
    juce::Font trackNameFont {14.0f, juce::Font::bold};  // Track name font
    juce::Path playheadTriangle;               // Cached playhead triangle
    bool playheadTriangleInitialized = false;

    #if JUCE_DEBUG
        bool showDebugOverlay = true;
        juce::Font debugFont {10.0f};
    #endif

    //==========================================================================
    // W6: Paint stats tracking (for StatsOverlay)
    //==========================================================================

    mutable int lastPaintVisibleTracks = 0;
    mutable int lastPaintVisibleClips = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackView)
};
