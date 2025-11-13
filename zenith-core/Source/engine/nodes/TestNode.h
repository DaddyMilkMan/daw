/**
 * @file TestNode.h
 * @brief Simple test node for verifying FX chain order
 *
 * DEBUG-only node for testing purposes.
 * Applies a simple predictable transformation to verify FX chain behavior.
 */

#pragma once

#include "../../../include/engine/IDspNode.h"
#include <JuceHeader.h>
#include <atomic>

namespace zenith {

/**
 * @brief Simple test DSP node
 *
 * Applies one of several simple transformations for testing:
 * - SignalInvert: Multiply all samples by -1.0
 * - GainDouble: Multiply all samples by 2.0
 * - GainHalf: Multiply all samples by 0.5
 * - AddConstant: Add 0.1 to all samples
 *
 * Used to verify:
 * - FX chain processes nodes in order
 * - Bypass works correctly
 * - Multiple FX slots can be chained
 */
class TestNode : public IDspNode
{
public:
    enum class TransformType
    {
        SignalInvert,   ///< Multiply by -1.0
        GainDouble,     ///< Multiply by 2.0
        GainHalf,       ///< Multiply by 0.5
        AddConstant     ///< Add 0.1
    };

    explicit TestNode(TransformType type);
    ~TestNode() override;

    //==========================================================================
    // IDspNode Interface
    //==========================================================================

    void prepareToPlay(double sampleRate, int blockSize, int numChannels) override;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) override;
    void releaseResources() override;
    void setBypassed(bool bypassed) override;
    bool isBypassed() const noexcept override;

private:
    TransformType type_;
    std::atomic<bool> bypassed_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestNode)
};

} // namespace zenith
