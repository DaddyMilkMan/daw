/**
 * @file GainPanNode.h
 * @brief Simple gain + pan processing node (internal FX, no VST3)
 *
 * Threading model:
 * - All setters: MESSAGE THREAD only
 * - processBlock: AUDIO THREAD only (RT-safe)
 * - All state is atomic for thread-safe reads
 */

#pragma once

#include "../../../include/engine/IDspNode.h"
#include <JuceHeader.h>
#include <atomic>

namespace zenith {

/**
 * @brief Gain + Pan processing node
 *
 * Applies gain and stereo panning with equal-power law:
 * - Mono input: Gain only
 * - Stereo input: Equal-power pan, then gain
 *
 * Parameters:
 * - Gain: [0.0 .. 2.0], linear amplitude multiplier
 * - Pan: [-1.0 .. +1.0], equal-power stereo balance
 *
 * RT-safe: No allocations, no locks, no logging in processBlock()
 */
class GainPanNode : public IDspNode
{
public:
    GainPanNode();
    ~GainPanNode() override;

    //==========================================================================
    // IDspNode Interface
    //==========================================================================

    void prepareToPlay(double sampleRate, int blockSize, int numChannels) override;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) override;
    void releaseResources() override;
    void setBypassed(bool bypassed) override;
    bool isBypassed() const noexcept override;

    //==========================================================================
    // Parameter Control (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Set gain in linear amplitude
     * @param gain Linear gain [0.0 .. 2.0]
     *
     * MESSAGE THREAD only (atomic store, safe for audio thread to read)
     */
    void setGain(float gain) noexcept;

    /**
     * @brief Get current gain
     * @return Linear gain [0.0 .. 2.0]
     *
     * Safe to call from any thread (atomic load)
     */
    float getGain() const noexcept;

    /**
     * @brief Set pan position
     * @param pan Pan position [-1.0 = full left, 0.0 = center, +1.0 = full right]
     *
     * MESSAGE THREAD only (atomic store, safe for audio thread to read)
     */
    void setPan(float pan) noexcept;

    /**
     * @brief Get current pan position
     * @return Pan position [-1.0 .. +1.0]
     *
     * Safe to call from any thread (atomic load)
     */
    float getPan() const noexcept;

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Calculate equal-power pan gains
     * @param pan Pan position [-1.0 .. +1.0]
     * @param leftGain Output: left channel gain
     * @param rightGain Output: right channel gain
     *
     * Uses constant-power panning law (sqrt):
     * - Pan = -1.0 → L=1.0, R=0.0
     * - Pan =  0.0 → L=0.707, R=0.707 (-3dB)
     * - Pan = +1.0 → L=0.0, R=1.0
     */
    static void calculatePanGains(float pan, float& leftGain, float& rightGain) noexcept;

    //==========================================================================
    // Member Variables (all atomic for thread-safe access)
    //==========================================================================

    std::atomic<float> gain_{1.0f};      ///< Linear gain [0.0 .. 2.0]
    std::atomic<float> pan_{0.0f};       ///< Pan position [-1.0 .. +1.0]
    std::atomic<bool> bypassed_{false};  ///< Bypass state

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPanNode)
};

} // namespace zenith
