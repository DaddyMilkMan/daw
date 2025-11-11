/**
 * @file AudioTrack.h
 * @brief Individual audio track in the mixer
 *
 * Represents a single track in the DAW with:
 * - Volume, pan, mute, solo controls
 * - Audio clips playback
 * - Effects chain (insert effects)
 * - Send routing to buses
 * - Input monitoring
 * - Real-time safe processing
 *
 * Thread Safety:
 * - process() runs on AUDIO THREAD (real-time safe!)
 * - Parameter changes use std::atomic or lock-free FIFO
 * - Never allocates memory in audio callback
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

//==============================================================================
/**
 * @class AudioTrack
 * @brief Individual track in the mixing console
 *
 * Each track can:
 * - Play back multiple audio clips
 * - Apply insert effects
 * - Route to send buses
 * - Solo/mute/volume/pan
 * - Monitor input while recording
 */
class AudioTrack
{
public:
    //==========================================================================
    AudioTrack(const juce::String& trackName, int trackIndex);
    ~AudioTrack();

    //==========================================================================
    // Processing (AUDIO THREAD - REAL-TIME SAFE!)
    //==========================================================================

    /**
     * @brief Process audio for this track
     * @param buffer Output buffer to write to
     * @param numSamples Number of samples to process
     * @param playheadPosition Current playback position in samples
     *
     * ⚠️ CRITICAL: This runs on AUDIO THREAD!
     * - No allocations
     * - No locks
     * - No system calls
     * - Use pre-allocated buffers only
     */
    void process(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 playheadPosition);

    //==========================================================================
    // Track Controls (MESSAGE THREAD)
    //==========================================================================

    void setVolume(float newVolume);
    float getVolume() const { return volume.load(); }

    void setPan(float newPan);
    float getPan() const { return pan.load(); }

    void setMute(bool shouldMute);
    bool isMuted() const { return muted.load(); }

    void setSolo(bool shouldSolo);
    bool isSoloed() const { return soloed.load(); }

    void setArmed(bool shouldArm);
    bool isArmed() const { return armed.load(); }

    //==========================================================================
    // Clip Management (MESSAGE THREAD)
    //==========================================================================

    struct AudioClip
    {
        juce::String id;
        juce::int64 startPosition;  // In samples
        juce::int64 length;          // In samples
        juce::AudioBuffer<float> audioData;
        float gain{1.0f};

        // Fade in/out
        int fadeInSamples{0};
        int fadeOutSamples{0};
    };

    /**
     * @brief Add an audio clip to this track
     * @param clip Clip to add
     */
    void addClip(const AudioClip& clip);

    /**
     * @brief Remove a clip by ID
     * @param clipId Clip ID to remove
     */
    void removeClip(const juce::String& clipId);

    /**
     * @brief Clear all clips
     */
    void clearClips();

    //==========================================================================
    // Info
    //==========================================================================

    juce::String getName() const { return name; }
    int getIndex() const { return index; }

    /**
     * @brief Get current audio level (for metering)
     * @return Peak level in dB
     */
    float getPeakLevel() const;

private:
    //==========================================================================
    // Helper Methods (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Render clips at current playhead position
     * @param buffer Buffer to write to
     * @param numSamples Number of samples
     * @param playheadPosition Current position in samples
     */
    void renderClips(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 playheadPosition);

    /**
     * @brief Apply gain and pan to buffer
     * @param buffer Buffer to process
     * @param numSamples Number of samples
     */
    void applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples);

    /**
     * @brief Apply fade in/out to a clip region
     */
    void applyFade(juce::AudioBuffer<float>& buffer, int startSample, int numSamples,
                   int fadeInSamples, int fadeOutSamples, int clipTotalLength);

    //==========================================================================
    // Member Variables
    //==========================================================================

    juce::String name;
    int index;

    // Track parameters (std::atomic for thread-safe access)
    std::atomic<float> volume{0.8f};   // 0.0 to 1.0
    std::atomic<float> pan{0.0f};      // -1.0 (left) to 1.0 (right)
    std::atomic<bool> muted{false};
    std::atomic<bool> soloed{false};
    std::atomic<bool> armed{false};

    // Audio clips on this track
    // NOTE: Clips are accessed from audio thread, so we need careful synchronization
    juce::CriticalSection clipLock;
    std::vector<AudioClip> clips;

    // Temporary buffer for mixing clips (pre-allocated, reused)
    juce::AudioBuffer<float> tempBuffer;

    // Level metering (updated in audio thread, read from UI thread)
    std::atomic<float> peakLevel{0.0f};

    // Sample rate (set when engine starts)
    std::atomic<double> sampleRate{44100.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTrack)
};
