/**
 * @file Mixer.h
 * @brief Central mixing engine for Zenith DAW
 *
 * The Mixer is the heart of the audio engine:
 * - Manages all audio tracks
 * - Sums track outputs to master bus
 * - Handles solo/mute logic
 * - Master volume and limiting
 * - CPU-efficient real-time mixing
 *
 * Thread Safety:
 * - process() runs on AUDIO THREAD (real-time safe!)
 * - Track management runs on MESSAGE THREAD
 * - Uses CriticalSection for track list access
 *
 * Architecture:
 * ```
 * [Track 1] ─┐
 * [Track 2] ─┼─> [Master Bus] ─> [Output]
 * [Track 3] ─┘
 * ```
 */

#pragma once

#include <JuceHeader.h>
#include "AudioTrack.h"
#include <vector>
#include <memory>
#include <atomic>

//==============================================================================
/**
 * @class Mixer
 * @brief Central audio mixing engine
 *
 * Responsibilities:
 * - Mix all tracks to master output
 * - Handle solo/mute logic
 * - Master volume control
 * - CPU monitoring
 * - Level metering
 */
class Mixer
{
public:
    //==========================================================================
    Mixer();
    ~Mixer();

    //==========================================================================
    // Processing (AUDIO THREAD - REAL-TIME SAFE!)
    //==========================================================================

    /**
     * @brief Process all tracks and mix to output
     * @param outputBuffer Output buffer to write to
     * @param numSamples Number of samples to process
     * @param playheadPosition Current playback position in samples
     *
     * ⚠️ CRITICAL: This runs on AUDIO THREAD!
     * - No allocations
     * - No locks (or only tryLock with fallback)
     * - No system calls
     */
    void process(juce::AudioBuffer<float>& outputBuffer, int numSamples, juce::int64 playheadPosition);

    //==========================================================================
    // Track Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Add a new track
     * @param name Track name
     * @return Pointer to created track (not owned - do not delete!)
     */
    AudioTrack* addTrack(const juce::String& name);

    /**
     * @brief Remove a track by index
     * @param index Track index
     */
    void removeTrack(int index);

    /**
     * @brief Get a track by index
     * @param index Track index
     * @return Pointer to track (not owned!)
     */
    AudioTrack* getTrack(int index);

    /**
     * @brief Get number of tracks
     */
    int getNumTracks() const;

    /**
     * @brief Clear all tracks
     */
    void clearAllTracks();

    //==========================================================================
    // Master Controls (MESSAGE THREAD)
    //==========================================================================

    void setMasterVolume(float newVolume);
    float getMasterVolume() const { return masterVolume.load(); }

    float getMasterPeakLevel() const;

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     * @brief Prepare mixer for playback
     * @param sampleRate Sample rate
     * @param maxBlockSize Maximum block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * @brief Release resources
     */
    void release();

private:
    //==========================================================================
    // Helper Methods (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Check solo/mute logic and process tracks
     * @param outputBuffer Output buffer
     * @param numSamples Number of samples
     * @param playheadPosition Current position
     */
    void processTracks(juce::AudioBuffer<float>& outputBuffer, int numSamples, juce::int64 playheadPosition);

    /**
     * @brief Apply master volume and limiting
     * @param buffer Buffer to process
     * @param numSamples Number of samples
     */
    void applyMasterProcessing(juce::AudioBuffer<float>& buffer, int numSamples);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Tracks
    juce::CriticalSection trackLock;
    std::vector<std::unique_ptr<AudioTrack>> tracks;

    // Master parameters
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> masterPeakLevel{0.0f};

    // Mixing buffer (pre-allocated, reused)
    juce::AudioBuffer<float> mixBuffer;

    // Audio settings
    std::atomic<double> currentSampleRate{44100.0};
    std::atomic<int> currentMaxBlockSize{512};

    // Solo state tracking
    std::atomic<bool> anySoloActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mixer)
};
