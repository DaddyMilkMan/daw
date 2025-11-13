/**
 * @file GainPanNode.h
 * @brief Internal gain/pan FX node for testing W11.0 plugin chain
 *
 * W11.0: Plugin Chain Scaffolding
 * ================================
 *
 * Purpose:
 * - Simple internal FX node to validate plugin chain infrastructure
 * - Demonstrates RT-safe parameter handling (atomics)
 * - Uses equal-power pan law (same as Engine master pan)
 *
 * Design:
 * - Zero RT allocations (no internal buffers needed)
 * - Atomic parameters for cross-thread safety
 * - Bypassed nodes return immediately (no processing)
 *
 * Future:
 * - This pattern will be extended for EQ, compressor, etc.
 * - VST3 instances will use same IDspNode interface
 */

#pragma once

#include "../IDspNode.h"
#include <atomic>

namespace zenith {

/**
 * @class GainPanNode
 * @brief Internal gain/pan processor (no RT allocations)
 *
 * Features:
 * - Gain: [0..2], default 1.0 (0 dB)
 * - Pan: [-1..1], L..R, default 0.0 (center)
 * - Equal-power pan law (constant power across pan range)
 * - Bypass support (zero-cost when bypassed)
 *
 * Thread Safety:
 * - setGain/setPan: MESSAGE THREAD (atomics for cross-thread access)
 * - processBlock: AUDIO THREAD (reads atomics with relaxed ordering)
 */
class GainPanNode : public IDspNode
{
public:
    GainPanNode() = default;
    ~GainPanNode() override = default;

    //==========================================================================
    // IDspNode interface
    //==========================================================================

    void prepareToPlay(double sampleRate, int blockSize, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) override;

    void setBypassed(bool shouldBypass) override
    {
        bypassed_.store(shouldBypass, std::memory_order_relaxed);
    }

    bool isBypassed() const override
    {
        return bypassed_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    // Parameter Control (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Set gain [0..2], default 1.0
     * @param g Gain multiplier (0 = silent, 1 = 0 dB, 2 = +6 dB)
     */
    void setGain(float g)
    {
        gain_.store(juce::jlimit(0.0f, 2.0f, g), std::memory_order_relaxed);
    }

    /**
     * @brief Get current gain
     */
    float getGain() const
    {
        return gain_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Set pan [-1..1], default 0.0 (center)
     * @param p Pan position (-1 = hard left, 0 = center, +1 = hard right)
     */
    void setPan(float p)
    {
        pan_.store(juce::jlimit(-1.0f, 1.0f, p), std::memory_order_relaxed);
    }

    /**
     * @brief Get current pan
     */
    float getPan() const
    {
        return pan_.load(std::memory_order_relaxed);
    }

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    // Parameters (atomics for cross-thread access)
    std::atomic<float> gain_{1.0f};      // [0..2]
    std::atomic<float> pan_{0.0f};       // [-1..1]
    std::atomic<bool> bypassed_{false};

    // Cached settings (updated in prepareToPlay)
    int currentNumChannels_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPanNode)
};

} // namespace zenith
