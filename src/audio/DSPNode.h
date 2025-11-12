/**
 * DSPNode.h
 * Base class for all DSP processing nodes in the audio graph
 *
 * Inspired by Web Audio API and modern DAW plugin architecture.
 * Each node can have multiple inputs/outputs and processes audio in place.
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <string>

namespace vexel {

/**
 * Abstract base class for DSP processing nodes
 */
class DSPNode
{
public:
    DSPNode(const juce::String& name, int numInputs = 2, int numOutputs = 2)
        : m_name(name)
        , m_numInputChannels(numInputs)
        , m_numOutputChannels(numOutputs)
    {}

    virtual ~DSPNode() = default;

    // ========================================================================
    // Core Processing
    // ========================================================================

    /**
     * Prepare for audio processing
     * Called before playback starts or when sample rate/buffer size changes
     */
    virtual void prepare(double sampleRate, int maximumBlockSize) {
        m_sampleRate = sampleRate;
        m_maximumBlockSize = maximumBlockSize;
    }

    /**
     * Process audio block (pure virtual - must be implemented)
     *
     * @param inputChannelData  Array of input channel pointers
     * @param outputChannelData Array of output channel pointers
     * @param numSamples        Number of samples to process
     */
    virtual void process(const float* const* inputChannelData,
                        float* const* outputChannelData,
                        int numSamples) = 0;

    /**
     * Reset internal state (e.g., filters, delays, envelopes)
     */
    virtual void reset() {}

    // ========================================================================
    // Configuration
    // ========================================================================

    const juce::String& getName() const { return m_name; }
    void setName(const juce::String& name) { m_name = name; }

    int getNumInputChannels() const { return m_numInputChannels; }
    int getNumOutputChannels() const { return m_numOutputChannels; }

    // ========================================================================
    // Parameters
    // ========================================================================

    /**
     * Set parameter by name
     * @param paramName Parameter identifier
     * @param value Normalized value 0.0 - 1.0
     */
    virtual void setParameter(const juce::String& paramName, float value) {
        juce::ignoreUnused(paramName, value);
    }

    /**
     * Get parameter value
     * @return Normalized value 0.0 - 1.0
     */
    virtual float getParameter(const juce::String& paramName) const {
        juce::ignoreUnused(paramName);
        return 0.0f;
    }

    /**
     * Get list of available parameters
     */
    virtual juce::StringArray getParameterNames() const {
        return {};
    }

    // ========================================================================
    // Bypass & Mute
    // ========================================================================

    void setBypassed(bool bypassed) { m_bypassed = bypassed; }
    bool isBypassed() const { return m_bypassed; }

    // ========================================================================
    // Latency
    // ========================================================================

    virtual int getLatencySamples() const { return 0; }

protected:
    juce::String m_name;
    int m_numInputChannels;
    int m_numOutputChannels;

    double m_sampleRate = 44100.0;
    int m_maximumBlockSize = 512;

    bool m_bypassed = false;

    /**
     * Helper: Copy input to output (passthrough)
     */
    void copyInputToOutput(const float* const* input, float* const* output,
                          int numSamples) const {
        for (int ch = 0; ch < juce::jmin(m_numInputChannels, m_numOutputChannels); ++ch) {
            juce::FloatVectorOperations::copy(output[ch], input[ch], numSamples);
        }
    }

    /**
     * Helper: Add input to output (mixing)
     */
    void addInputToOutput(const float* const* input, float* const* output,
                         int numSamples) const {
        for (int ch = 0; ch < juce::jmin(m_numInputChannels, m_numOutputChannels); ++ch) {
            juce::FloatVectorOperations::add(output[ch], input[ch], numSamples);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPNode)
};

} // namespace vexel
