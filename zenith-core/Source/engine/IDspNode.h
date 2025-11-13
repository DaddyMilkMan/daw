/**
 * @file IDspNode.h
 * @brief Interface for track DSP nodes (internal + VST3)
 *
 * W11.0: Plugin Chain Scaffolding
 * ================================
 *
 * Purpose:
 * - Unified interface for internal FX (GainPanNode, EQ, etc.) and VST3 instances
 * - RT-safe processing contract (zero allocations in processBlock)
 * - Simple lifecycle matching JUCE audio processing patterns
 *
 * Design:
 * - Pure virtual interface (no JUCE AudioProcessor yet)
 * - Track owns nodes via std::unique_ptr (deterministic destruction)
 * - prepareToPlay → processBlock → releaseResources lifecycle
 * - Atomic bypass flag for zero-latency muting
 *
 * Thread Safety:
 * - prepareToPlay / releaseResources: MESSAGE THREAD
 * - processBlock: AUDIO THREAD (RT-safe!)
 * - setBypassed: MESSAGE THREAD (atomics allow safe reads in processBlock)
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * @interface IDspNode
 * @brief Base interface for all DSP nodes in Track FX chain
 *
 * Contract:
 * - prepareToPlay() called before audio starts (pre-allocate buffers)
 * - processBlock() called on AUDIO THREAD (never allocate, lock, or syscall)
 * - releaseResources() called when audio stops (cleanup)
 * - Bypass state can be changed from MESSAGE THREAD, read from AUDIO THREAD
 */
class IDspNode
{
public:
    virtual ~IDspNode() = default;

    //==========================================================================
    // Lifecycle (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Prepare node for audio processing
     * @param sampleRate Engine sample rate
     * @param blockSize Maximum block size
     * @param numChannels Number of channels (typically 2 for stereo)
     *
     * @note MESSAGE THREAD - can allocate, resize buffers, etc.
     */
    virtual void prepareToPlay(double sampleRate,
                               int blockSize,
                               int numChannels) = 0;

    /**
     * @brief Release resources when audio stops
     *
     * @note MESSAGE THREAD - safe to deallocate
     */
    virtual void releaseResources() = 0;

    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Process audio block in-place
     * @param buffer Audio buffer to process (modified in-place)
     * @param numSamples Number of samples to process (may be < blockSize)
     *
     * @note AUDIO THREAD - MUST BE RT-SAFE!
     * @note NEVER allocate, lock, or make syscalls here
     * @note If bypassed, this should be a no-op (early return)
     */
    virtual void processBlock(juce::AudioBuffer<float>& buffer,
                              int numSamples) = 0;

    //==========================================================================
    // Parameter Control (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Set bypass state (MESSAGE THREAD)
     * @param shouldBypass true = bypass (no processing), false = active
     *
     * @note Implementation should use std::atomic for RT-safe reads in processBlock
     */
    virtual void setBypassed(bool shouldBypass) = 0;

    /**
     * @brief Check if node is bypassed
     */
    virtual bool isBypassed() const = 0;
};

} // namespace zenith
