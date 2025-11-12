/**
 * @file Track.h
 * @brief Audio track for Zenith DAW
 *
 * Responsibilities:
 * - Manage track-level state (gain, pan, mute, solo)
 * - Render clips into audio buffer
 * - No allocations in processBlock() (real-time safe)
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "Clip.h"

namespace zenith {

/**
 * @class Track
 * @brief Single audio track with clips
 *
 * Thread Safety:
 * - processBlock() runs on AUDIO THREAD (real-time safe!)
 * - All setters run on MESSAGE THREAD
 * - Use std::atomic for cross-thread communication
 */
class Track
{
public:
    Track();
    ~Track();

    //==========================================================================
    // Lifecycle (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Prepare track for playback (pre-allocate buffers)
     * @param blockSize Maximum block size
     * @param sampleRate Sample rate
     */
    void prepareToPlay(int blockSize, double sampleRate);

    /**
     * @brief Release resources (called when audio stops)
     */
    void releaseResources();

    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Render track into mix buffer
     * @param mixBuffer Buffer to render into (additive mix)
     * @param numSamples Number of samples to render
     * @param transportPosition Current transport position in samples
     *
     * @note AUDIO THREAD - real-time safe!
     * @note Does NOT clear buffer - mixes additively
     */
    void processBlock(juce::AudioBuffer<float>& mixBuffer,
                      int numSamples,
                      juce::int64 transportPosition);

    //==========================================================================
    // Track State (MESSAGE THREAD)
    //==========================================================================

    void setName(const juce::String& name) { trackName = name; }
    juce::String getName() const { return trackName; }

    void setGain(float g) { gain_.store(juce::jlimit(0.0f, 2.0f, g)); }
    float getGain() const { return gain_.load(); }

    void setPan(float p) { pan_.store(juce::jlimit(-1.0f, 1.0f, p)); }
    float getPan() const { return pan_.load(); }

    void setMuted(bool m) { muted_.store(m); }
    bool isMuted() const { return muted_.load(); }

    void setSoloed(bool s) { soloed_.store(s); }
    bool isSoloed() const { return soloed_.load(); }

    //==========================================================================
    // W13: Clip Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Add clip to track (MESSAGE THREAD)
     * @param clip Clip to add (must be valid)
     *
     * @note Clips are automatically sorted by startSample
     * @note Do NOT call while audio is playing (static timeline assumption)
     */
    void addClip(const Clip& clip);

    /**
     * @brief Get number of clips
     */
    int getNumClips() const { return static_cast<int>(clips_.size()); }

    /**
     * @brief Clear all clips (MESSAGE THREAD)
     */
    void clearClips() { clips_.clear(); }

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    // Track metadata
    juce::String trackName;

    // Track state (atomics for thread-safe access)
    std::atomic<float> gain_{1.0f};     // [0..2], default 1.0
    std::atomic<float> pan_{0.0f};      // [-1..1], L..R, default center
    std::atomic<bool> muted_{false};
    std::atomic<bool> soloed_{false};

    // Audio settings
    double currentSampleRate{44100.0};
    int currentBlockSize{512};

    // Pre-allocated scratch buffer for track processing
    juce::AudioBuffer<float> trackBuffer;

    // W13: Clip list (sorted by startSample, MESSAGE THREAD access only)
    std::vector<Clip> clips_;

    // TODO: Add plugin chain, automation, etc.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
