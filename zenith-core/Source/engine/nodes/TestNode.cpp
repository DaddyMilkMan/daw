/**
 * @file TestNode.cpp
 * @brief Test node implementation
 */

#include "TestNode.h"

namespace zenith {

TestNode::TestNode(TransformType type)
    : type_(type)
{
}

TestNode::~TestNode()
{
}

//==============================================================================
// IDspNode Interface
//==============================================================================

void TestNode::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    juce::ignoreUnused(sampleRate, blockSize, numChannels);
    // No state to initialize - stateless processing
}

void TestNode::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Early exit if bypassed
    if (bypassed_.load(std::memory_order_acquire))
        return;

    const int numChannels = buffer.getNumChannels();
    if (numChannels == 0 || numSamples <= 0)
        return;

    // Apply transformation based on type
    switch (type_)
    {
        case TransformType::SignalInvert:
        {
            // Multiply all samples by -1.0
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* chan = buffer.getWritePointer(ch, 0);
                juce::FloatVectorOperations::multiply(chan, -1.0f, numSamples);
            }
            break;
        }

        case TransformType::GainDouble:
        {
            // Multiply all samples by 2.0
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* chan = buffer.getWritePointer(ch, 0);
                juce::FloatVectorOperations::multiply(chan, 2.0f, numSamples);
            }
            break;
        }

        case TransformType::GainHalf:
        {
            // Multiply all samples by 0.5
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* chan = buffer.getWritePointer(ch, 0);
                juce::FloatVectorOperations::multiply(chan, 0.5f, numSamples);
            }
            break;
        }

        case TransformType::AddConstant:
        {
            // Add 0.1 to all samples
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* chan = buffer.getWritePointer(ch, 0);
                juce::FloatVectorOperations::add(chan, 0.1f, numSamples);
            }
            break;
        }
    }
}

void TestNode::releaseResources()
{
    // No resources to release
}

void TestNode::setBypassed(bool bypassed)
{
    bypassed_.store(bypassed, std::memory_order_release);
}

bool TestNode::isBypassed() const noexcept
{
    return bypassed_.load(std::memory_order_acquire);
}

} // namespace zenith
