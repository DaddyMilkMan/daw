/**
 * @file GainPanNode.cpp
 * @brief Gain + Pan node implementation
 */

#include "GainPanNode.h"
#include <cmath>

namespace zenith {

GainPanNode::GainPanNode()
{
}

GainPanNode::~GainPanNode()
{
}

//==============================================================================
// IDspNode Interface
//==============================================================================

void GainPanNode::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    juce::ignoreUnused(sampleRate, blockSize, numChannels);
    // No internal state to allocate - stateless processing
}

void GainPanNode::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Early exit if bypassed
    if (bypassed_.load(std::memory_order_acquire))
        return;

    const int numChannels = buffer.getNumChannels();
    if (numChannels == 0 || numSamples <= 0)
        return;

    // Load parameters (atomic reads, safe on RT thread)
    const float gain = gain_.load(std::memory_order_acquire);
    const float pan = pan_.load(std::memory_order_acquire);

    if (numChannels == 1)
    {
        // Mono: Apply gain only
        float* mono = buffer.getWritePointer(0, 0);
        juce::FloatVectorOperations::multiply(mono, gain, numSamples);
    }
    else if (numChannels >= 2)
    {
        // Stereo: Apply equal-power pan, then gain
        float leftPanGain, rightPanGain;
        calculatePanGains(pan, leftPanGain, rightPanGain);

        float* left = buffer.getWritePointer(0, 0);
        float* right = buffer.getWritePointer(1, 0);

        const float leftTotal = leftPanGain * gain;
        const float rightTotal = rightPanGain * gain;

        juce::FloatVectorOperations::multiply(left, leftTotal, numSamples);
        juce::FloatVectorOperations::multiply(right, rightTotal, numSamples);

        // Additional channels (if any): apply gain only
        for (int ch = 2; ch < numChannels; ++ch)
        {
            float* chan = buffer.getWritePointer(ch, 0);
            juce::FloatVectorOperations::multiply(chan, gain, numSamples);
        }
    }
}

void GainPanNode::releaseResources()
{
    // No resources to release - stateless processing
}

void GainPanNode::setBypassed(bool bypassed)
{
    bypassed_.store(bypassed, std::memory_order_release);
}

bool GainPanNode::isBypassed() const noexcept
{
    return bypassed_.load(std::memory_order_acquire);
}

//==============================================================================
// Parameter Control
//==============================================================================

void GainPanNode::setGain(float gain) noexcept
{
    // Clamp to safe range [0.0 .. 2.0]
    gain = juce::jlimit(0.0f, 2.0f, gain);
    gain_.store(gain, std::memory_order_release);
}

float GainPanNode::getGain() const noexcept
{
    return gain_.load(std::memory_order_acquire);
}

void GainPanNode::setPan(float pan) noexcept
{
    // Clamp to valid range [-1.0 .. +1.0]
    pan = juce::jlimit(-1.0f, 1.0f, pan);
    pan_.store(pan, std::memory_order_release);
}

float GainPanNode::getPan() const noexcept
{
    return pan_.load(std::memory_order_acquire);
}

//==============================================================================
// Internal Helpers
//==============================================================================

void GainPanNode::calculatePanGains(float pan, float& leftGain, float& rightGain) noexcept
{
    // Clamp pan to valid range
    pan = juce::jlimit(-1.0f, 1.0f, pan);

    // Equal-power panning law using sin/cos
    // Pan angle: 0 = center, -PI/4 = full left, +PI/4 = full right
    const float piOver4 = juce::MathConstants<float>::pi / 4.0f;
    const float angle = pan * piOver4;

    // Constant-power pan law
    leftGain = std::cos(angle);
    rightGain = std::sin(angle);
}

} // namespace zenith
