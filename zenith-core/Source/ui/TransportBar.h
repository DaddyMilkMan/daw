/**
 * @file TransportBar.h
 * @brief Transport controls component for Zenith DAW
 *
 * Replaces React component:
 * - vexel-daw/src/renderer/components/TransportBar.tsx
 *
 * Features:
 * - Play/Pause/Stop/Record buttons
 * - Loop toggle
 * - Metronome toggle
 * - Tap tempo
 * - BPM control
 * - Position display (bars.beats.sixteenths and timecode)
 * - CPU meter
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"
#include "../../include/Engine.h"

//==============================================================================
/**
 * @class TransportBar
 * @brief Bottom transport bar with playback controls
 *
 * Layout:
 * [Play] [Stop] [Record] | [Loop] [Metro] [Tap] | BPM: [120.0] | 1.1.1 | 00:00:00:00 | CPU: 12%
 */
class TransportBar : public juce::Component,
                     private juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Callbacks for transport actions
     */
    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void()> onRecord;
    std::function<void(bool)> onLoopToggle;
    std::function<void(bool)> onMetronomeToggle;
    std::function<void(double)> onBPMChanged;

    //==========================================================================
    explicit TransportBar(Engine& engine);
    ~TransportBar() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Public API
    //==========================================================================

    void setPosition(double positionInQuarterNotes);
    void setBPM(double bpm);
    void setPlaying(bool isPlaying);
    void setRecording(bool isRecording);
    void setLoopEnabled(bool enabled);
    void setMetronomeEnabled(bool enabled);

private:
    //==========================================================================
    // Timer callback
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Button callbacks
    //==========================================================================

    void playButtonClicked();
    void stopButtonClicked();
    void recordButtonClicked();
    void loopButtonClicked();
    void metronomeButtonClicked();
    void tapTempoButtonClicked();

    //==========================================================================
    // Helper methods
    //==========================================================================

    juce::String formatPosition(double positionInQuarterNotes);
    juce::String formatTimecode(double positionInSeconds);

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;

    // Transport buttons
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;

    // Toggle buttons
    juce::TextButton loopButton;
    juce::TextButton metronomeButton;
    juce::TextButton tapTempoButton;

    // BPM control
    juce::Label bpmLabel;
    juce::Slider bpmSlider;

    // Position displays
    juce::Label positionLabel;
    juce::Label timecodeLabel;

    // CPU meter
    juce::Label cpuLabel;

    // State
    double currentBPM = 120.0;
    double currentPosition = 0.0;
    bool isPlaying = false;
    bool isRecording = false;
    bool loopEnabled = false;
    bool metronomeEnabled = false;

    // Tap tempo
    std::vector<juce::int64> tapTimes;
    static constexpr int maxTaps = 4;
    static constexpr int tapTimeoutMs = 2000;

    //==========================================================================
    // W4: Cached resources for text rendering optimization
    //==========================================================================

    juce::Font bpmFont {12.0f};                    // Cached BPM label font
    juce::Font positionFont {16.0f, juce::Font::bold}; // Cached position font
    juce::Font timecodeFont {11.0f};               // Cached timecode font
    juce::Font cpuFont {12.0f};                    // Cached CPU label font

    // Dirty-check state to avoid unnecessary setText() calls
    double lastDisplayedCpuUsage = -1.0;
    double lastDisplayedPosition = -1.0;
    juce::String lastCpuText;
    juce::String lastPositionText;
    juce::String lastTimecodeText;

    // Pre-allocated buffer for timecode formatting (avoid allocation in timer)
    char timecodeBuffer[16];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
