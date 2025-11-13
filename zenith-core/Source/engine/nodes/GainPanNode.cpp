/**
 * @file GainPanNode.cpp
 * @brief Implementation of internal gain/pan FX node
 */

#include "GainPanNode.h"

using namespace zenith;

//==============================================================================
void GainPanNode::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    juce::ignoreUnused(sampleRate, blockSize);

    // Cache channel count for processBlock
    currentNumChannels_ = numChannels;

    DBG("GainPanNode: Prepared ("
        + juce::String(numChannels) + " ch, "
        + juce::String(blockSize) + " samples, "
        + juce::String(sampleRate, 0) + " Hz)");
}

void GainPanNode::releaseResources()
{
    // No resources to release (no internal buffers)
    DBG("GainPanNode: Released");
}

void GainPanNode::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
    //
    // NEVER:
    // - Allocate memory
    // - Lock mutexes
    // - Make system calls (DBG, file I/O, etc.)

    // Early exit if bypassed
    if (bypassed_.load(std::memory_order_relaxed))
        return;

    // Read parameters (constant per block)
    const float g = gain_.load(std::memory_order_relaxed);
    const float p = pan_.load(std::memory_order_relaxed);

    const int numChannels = buffer.getNumChannels();

    if (numChannels == 1)
    {
        // Mono: apply gain only
        float* data = buffer.getWritePointer(0);
        juce::FloatVectorOperations::multiply(data, g, numSamples);
    }
    else if (numChannels >= 2)
    {
        // Stereo: apply gain + equal-power pan
        //
        // Equal-power pan law (same as Engine master pan):
        // - pan = -1.0 → L = 1.0, R = 0.0 (hard left)
        // - pan =  0.0 → L = 0.5, R = 0.5 (center, -3 dB)
        // - pan = +1.0 → L = 0.0, R = 1.0 (hard right)
        //
        // Formula:
        //   l = 0.5 - 0.5 * pan  (range [0..1])
        //   r = 0.5 + 0.5 * pan  (range [0..1])
        //   gL = gain * l * 2.0
        //   gR = gain * r * 2.0

        const float l = juce::jlimit(0.0f, 1.0f, 0.5f - 0.5f * p);
        const float r = juce::jlimit(0.0f, 1.0f, 0.5f + 0.5f * p);

        const float gL = g * l * 2.0f;
        const float gR = g * r * 2.0f;

        // Apply to L/R channels
        float* dataL = buffer.getWritePointer(0);
        float* dataR = buffer.getWritePointer(1);

        juce::FloatVectorOperations::multiply(dataL, gL, numSamples);
        juce::FloatVectorOperations::multiply(dataR, gR, numSamples);

        // Clear any extra channels (should not happen, but safety)
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
}
