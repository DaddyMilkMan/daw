/**
 * @file IDspNode.h
 * @brief Pure virtual interface for DSP processing nodes (internal FX, VST3 wrappers, etc.)
 *
 * Threading model:
 * - prepareToPlay / releaseResources / setBypassed: MESSAGE THREAD only
 * - processBlock: AUDIO THREAD only
 * - isBypassed: Both threads (atomic read)
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * @brief Abstract DSP processing node interface
 *
 * Lifecycle:
 * 1. Construction (message thread)
 * 2. prepareToPlay() called when audio device starts
 * 3. processBlock() called on audio thread for each block
 * 4. releaseResources() called when audio device stops
 * 5. Destruction (message thread)
 *
 * No parameters, no automation yet - just pure audio processing.
 */
class IDspNode
{
public:
    virtual ~IDspNode() = default;

    /**
     * @brief Prepare for playback (MESSAGE THREAD)
     * @param sampleRate Sample rate in Hz
     * @param blockSize Maximum expected block size
     * @param numChannels Number of channels (1=mono, 2=stereo)
     *
     * Called when audio device starts or format changes.
     * Node should allocate internal buffers, reset state, etc.
     */
    virtual void prepareToPlay(double sampleRate, int blockSize, int numChannels) = 0;

    /**
     * @brief Process audio block (AUDIO THREAD - RT SAFE)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples to process (may be < buffer.getNumSamples())
     *
     * CRITICAL CONSTRAINTS:
     * - NO allocations
     * - NO locks
     * - NO logging
     * - Process in-place: read from buffer, write results back to buffer
     * - If bypassed, must return immediately without processing
     *
     * Buffer ownership: Caller owns buffer, node processes in-place.
     */
    virtual void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) = 0;

    /**
     * @brief Release resources (MESSAGE THREAD)
     *
     * Called when audio device stops.
     * Node should free internal buffers, reset state.
     */
    virtual void releaseResources() = 0;

    /**
     * @brief Set bypass state (MESSAGE THREAD)
     * @param bypassed True to bypass processing
     */
    virtual void setBypassed(bool bypassed) = 0;

    /**
     * @brief Get bypass state (ANY THREAD - atomic read)
     * @return True if node is bypassed
     */
    virtual bool isBypassed() const noexcept = 0;
};

} // namespace zenith
