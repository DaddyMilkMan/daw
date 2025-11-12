/**
 * @file Clip.h
 * @brief Audio clip for timeline rendering
 *
 * Responsibilities:
 * - Store pre-decoded PCM audio data
 * - Define timeline position and playback parameters
 * - Support sample-accurate rendering with fades
 *
 * Thread Safety:
 * - Clips are immutable during playback (MESSAGE THREAD creates, AUDIO THREAD reads)
 * - PCM data shared via shared_ptr (reference counted, thread-safe)
 */

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace zenith {

/**
 * @struct Clip
 * @brief Audio clip with pre-decoded PCM data
 *
 * Design:
 * - Immutable during playback (created on MESSAGE THREAD)
 * - PCM data shared via shared_ptr (zero-copy sharing)
 * - Sample-accurate timeline positioning
 * - Linear fade in/out support
 *
 * Memory:
 * - PCM buffer allocated on MESSAGE THREAD (pre-decode)
 * - No allocations in AUDIO THREAD (read-only access)
 */
struct Clip
{
    //==========================================================================
    // Audio Data
    //==========================================================================

    /**
     * @brief Pre-decoded PCM audio data (shared ownership)
     *
     * - Allocated on MESSAGE THREAD (file decode)
     * - Shared across clips (zero-copy)
     * - Read-only in AUDIO THREAD
     */
    std::shared_ptr<juce::AudioBuffer<float>> pcm;

    //==========================================================================
    // Timeline Position
    //==========================================================================

    /**
     * @brief Clip start position on timeline (samples)
     */
    juce::int64 startSample = 0;

    /**
     * @brief Clip length in samples
     */
    juce::int64 lengthSamples = 0;

    /**
     * @brief Offset into PCM buffer (for clip trimming)
     */
    juce::int64 srcOffset = 0;

    //==========================================================================
    // Playback Parameters
    //==========================================================================

    /**
     * @brief Clip gain [0..2], default 1.0
     */
    float gain = 1.0f;

    /**
     * @brief Fade-in duration (samples)
     */
    int fadeInSamples = 0;

    /**
     * @brief Fade-out duration (samples)
     */
    int fadeOutSamples = 0;

    /**
     * @brief Loop playback (not yet implemented)
     */
    bool loop = false;

    //==========================================================================
    // Computed Properties
    //==========================================================================

    /**
     * @brief Get clip end position (startSample + lengthSamples)
     */
    juce::int64 getEndSample() const noexcept
    {
        return startSample + lengthSamples;
    }

    /**
     * @brief Check if clip intersects block [blockStart, blockStart + blockSize)
     */
    bool intersectsBlock(juce::int64 blockStart, int blockSize) const noexcept
    {
        const juce::int64 blockEnd = blockStart + blockSize;
        return startSample < blockEnd && getEndSample() > blockStart;
    }

    /**
     * @brief Check if clip is valid for playback
     */
    bool isValid() const noexcept
    {
        return pcm != nullptr
            && pcm->getNumChannels() > 0
            && pcm->getNumSamples() > 0
            && lengthSamples > 0
            && srcOffset >= 0
            && srcOffset + lengthSamples <= pcm->getNumSamples();
    }
};

/**
 * @class ClipLoader
 * @brief Helper for loading audio files into Clip objects
 *
 * Usage:
 *   ClipLoader loader;
 *   auto clip = loader.loadFromFile(File("audio.wav"), startSample);
 *   track->addClip(clip);
 */
class ClipLoader
{
public:
    ClipLoader();
    ~ClipLoader();

    /**
     * @brief Load audio file into Clip (MESSAGE THREAD)
     * @param file Audio file to load (WAV, AIFF, FLAC, etc.)
     * @param startSample Timeline position for clip start
     * @param fadeInMs Fade-in duration in milliseconds (0 = no fade)
     * @param fadeOutMs Fade-out duration in milliseconds (0 = no fade)
     * @return Clip with pre-decoded PCM, or invalid Clip on error
     *
     * @note This decodes the entire file into memory (MESSAGE THREAD)
     * @note Do NOT call from AUDIO THREAD
     */
    Clip loadFromFile(const juce::File& file,
                      juce::int64 startSample = 0,
                      double fadeInMs = 0.0,
                      double fadeOutMs = 0.0);

private:
    juce::AudioFormatManager formatManager;
    double currentSampleRate = 44100.0;  // Updated when loading

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipLoader)
};

} // namespace zenith
