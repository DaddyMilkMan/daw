/**
 * @file TrackView.h
 * @brief Central arrangement/track view for Zenith DAW
 *
 * Replaces React components:
 * - vexel-daw/src/renderer/components/ArrangementView.tsx (main arrangement)
 * - vexel-daw/src/renderer/components/Timeline.tsx (timeline ruler)
 * - vexel-daw/src/renderer/components/TrackList.tsx (track headers)
 *
 * Features:
 * - Timeline ruler with bar/beat markers
 * - Multiple audio/MIDI tracks
 * - Audio clips with waveform display
 * - MIDI clips with note preview
 * - Drag-and-drop clip manipulation
 * - Zoom controls (horizontal/vertical)
 * - Playhead cursor
 * - Loop region markers
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

//==============================================================================
/**
 * @class TrackView
 * @brief Main arrangement view with timeline and tracks
 *
 * Layout:
 * [Timeline Ruler]
 * [Track 1 Header | Track 1 Clips -------------------------->]
 * [Track 2 Header | Track 2 Clips -------------------------->]
 * [Track 3 Header | Track 3 Clips -------------------------->]
 */
class TrackView : public juce::Component,
                  private juce::Timer
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

    //==========================================================================
    TrackView();
    ~TrackView() override;

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
     * @brief Set playhead position (in quarter notes)
     */
    void setPlayheadPosition(double positionInQuarterNotes);

    /**
     * @brief Set BPM (affects grid/ruler)
     */
    void setBPM(double bpm);

    /**
     * @brief Set horizontal zoom (pixels per quarter note)
     */
    void setHorizontalZoom(float zoom);

    /**
     * @brief Set vertical zoom (track height multiplier)
     */
    void setVerticalZoom(float zoom);

    /**
     * @brief Add a track
     */
    void addTrack(const juce::String& trackName);

    /**
     * @brief Remove a track
     */
    void removeTrack(int trackIndex);

    /**
     * @brief Set loop region
     */
    void setLoopRegion(double startInQuarterNotes, double endInQuarterNotes);

private:
    //==========================================================================
    // Timer callback (for playhead animation)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Drawing helpers
    //==========================================================================

    void drawTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);

    /**
     * @brief Convert time (quarter notes) to X coordinate
     */
    float timeToX(double timeInQuarterNotes) const;

    /**
     * @brief Convert X coordinate to time (quarter notes)
     */
    double xToTime(float x) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    // Track data
    juce::StringArray trackNames;

    // Playback state
    double playheadPosition = 0.0;  // In quarter notes
    double currentBPM = 120.0;
    bool isPlaying = false;

    // Loop region
    bool loopEnabled = false;
    double loopStart = 0.0;
    double loopEnd = 16.0;

    // View state
    float horizontalZoom = 40.0f;   // Pixels per quarter note
    float verticalZoom = 1.0f;      // Track height multiplier
    float scrollX = 0.0f;           // Horizontal scroll offset
    float scrollY = 0.0f;           // Vertical scroll offset

    // Constants
    static constexpr int timelineHeight = 32;
    static constexpr int trackHeaderWidth = 150;
    static constexpr int defaultTrackHeight = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackView)
};
