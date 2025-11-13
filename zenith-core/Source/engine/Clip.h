/**
 * @file Clip.h
 * @brief Audio clip data structure (PCM + metadata)
 *
 * Holds decoded PCM audio and playback parameters.
 * Message thread creates/modifies, audio thread reads (shared_ptr).
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <cstdint>

namespace zenith {

/**
 * @brief Audio clip with PCM data and playback parameters
 *
 * Thread safety:
 * - Created/modified on MESSAGE THREAD only
 * - Audio thread accesses via shared_ptr (read-only)
 * - No modifications allowed once scheduled for playback
 */
struct Clip
{
    //==========================================================================
    // PCM Data
    //==========================================================================

    /// Decoded audio samples (shared ownership, thread-safe via shared_ptr)
    std::shared_ptr<juce::AudioBuffer<float>> pcm;

    //==========================================================================
    // Timeline Position
    //==========================================================================

    /// Timeline position where clip starts (samples)
    int64_t startSample = 0;

    /// Length of clip in samples
    int64_t lengthSamples = 0;

    /// Offset into source file (for trimming start)
    int64_t srcOffset = 0;

    //==========================================================================
    // Playback Parameters
    //==========================================================================

    /// Clip gain multiplier (linear, 1.0 = unity)
    float gain = 1.0f;

    /// Fade-in duration (samples)
    int fadeInSamples = 0;

    /// Fade-out duration (samples)
    int fadeOutSamples = 0;

    /// Loop playback (not implemented in Phase 1)
    bool loop = false;

    //==========================================================================
    // Validation
    //==========================================================================

    /**
     * @brief Check if clip has valid PCM data
     * @return true if clip can be played
     */
    bool isValid() const noexcept
    {
        return pcm != nullptr
            && pcm->getNumChannels() > 0
            && pcm->getNumSamples() > 0
            && lengthSamples > 0;
    }

    /**
     * @brief Get actual playable length accounting for source offset
     * @return Length in samples
     */
    int64_t getPlayableLengthSamples() const noexcept
    {
        if (!pcm) return 0;
        return juce::jmin(lengthSamples,
                          static_cast<int64_t>(pcm->getNumSamples()) - srcOffset);
    }
};

} // namespace zenith
