/*
  ==============================================================================

    Metronome.h
    Created: 2025-12-18
    Author:  Zenith DAW

    Synthesized metronome for click track generation.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace zenith {

class TempoMap;

class Metronome
{
public:
    Metronome();
    ~Metronome() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    /**
     * @brief Generates the click track audio.
     * @param bufferToFill The buffer to mix the click into.
     * @param currentTransportSample The current absolute sample position of the transport.
     * @param isPlaying Whether the transport is currently playing.
     * @param tempoMap The TempoMap to calculate beat positions.
     */
    void getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill, 
                           int64_t currentTransportSample, 
                           bool isPlaying,
                           const TempoMap& tempoMap);

    void setEnabled(bool shouldBeEnabled);
    bool isEnabled() const;

    void setLevel(float newLevel);
    float getLevel() const;

private:
    double sampleRate_ = 44100.0;
    std::atomic<bool> enabled_{false};
    std::atomic<float> level_{0.5f}; // -6dB default

    // Synthesis state
    int currentNoteSamplesRemaining_ = 0;
    float currentFrequency_ = 0.0f;
    float currentPhase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    
    // Constants
    static constexpr float kHighClickFreq = 1600.0f;
    static constexpr float kLowClickFreq = 800.0f;
    static constexpr float kClickDurationSec = 0.1f;
    static constexpr float kReleaseTimeSec = 0.05f; // Short decay

    // Helper to trigger a click
    void triggerClick(float frequency);
};

} // namespace zenith
