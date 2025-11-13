/**
 * OscillatorNode.cpp
 * Implementation of multi-waveform oscillator
 */

#include "OscillatorNode.h"

namespace vexel {

OscillatorNode::OscillatorNode()
    : DSPNode("Oscillator", 0, 2)  // 0 inputs, 2 outputs (stereo)
{
}

void OscillatorNode::prepare(double sampleRate, int maximumBlockSize)
{
    DSPNode::prepare(sampleRate, maximumBlockSize);
    updatePhaseIncrement();
    m_random.setSeedRandomly();
}

void OscillatorNode::process(const float* const* input, float* const* output, int numSamples)
{
    juce::ignoreUnused(input);

    if (m_bypassed) {
        for (int ch = 0; ch < m_numOutputChannels; ++ch) {
            juce::FloatVectorOperations::clear(output[ch], numSamples);
        }
        return;
    }

    for (int sample = 0; sample < numSamples; ++sample) {
        float oscValue = generateSample();

        // Write to all output channels (mono->stereo)
        for (int ch = 0; ch < m_numOutputChannels; ++ch) {
            output[ch][sample] = oscValue * m_amplitude;
        }

        // Advance phase
        m_phase += m_phaseIncrement;
        if (m_phase >= juce::MathConstants<float>::twoPi) {
            m_phase -= juce::MathConstants<float>::twoPi;
        }
    }
}

void OscillatorNode::reset()
{
    m_phase = 0.0f;
}

void OscillatorNode::setParameter(const juce::String& paramName, float value)
{
    if (paramName == "frequency") {
        setFrequency(20.0f + value * 20000.0f);  // 20Hz - 20kHz
    }
    else if (paramName == "amplitude") {
        setAmplitude(value);
    }
    else if (paramName == "waveform") {
        int waveIndex = static_cast<int>(value * 4.99f);  // 0-4
        setWaveform(static_cast<OscillatorWaveform>(waveIndex));
    }
}

float OscillatorNode::getParameter(const juce::String& paramName) const
{
    if (paramName == "frequency") {
        return (m_frequency - 20.0f) / 19980.0f;
    }
    else if (paramName == "amplitude") {
        return m_amplitude;
    }
    else if (paramName == "waveform") {
        return static_cast<float>(m_waveform) / 4.0f;
    }
    return 0.0f;
}

juce::StringArray OscillatorNode::getParameterNames() const
{
    return { "frequency", "amplitude", "waveform" };
}

void OscillatorNode::updatePhaseIncrement()
{
    m_phaseIncrement = juce::MathConstants<float>::twoPi * m_frequency / static_cast<float>(m_sampleRate);
}

float OscillatorNode::generateSample()
{
    switch (m_waveform) {
        case OscillatorWaveform::Sine:
            return std::sin(m_phase);

        case OscillatorWaveform::Saw:
            return (m_phase / juce::MathConstants<float>::pi) - 1.0f;

        case OscillatorWaveform::Square:
            return (m_phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

        case OscillatorWaveform::Triangle: {
            float halfPi = juce::MathConstants<float>::pi;
            if (m_phase < halfPi) {
                return (m_phase / halfPi) * 2.0f - 1.0f;
            } else {
                return 1.0f - ((m_phase - halfPi) / halfPi) * 2.0f;
            }
        }

        case OscillatorWaveform::Noise:
            return m_random.nextFloat() * 2.0f - 1.0f;

        default:
            return 0.0f;
    }
}

} // namespace vexel
