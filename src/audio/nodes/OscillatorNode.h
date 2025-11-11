/**
 * OscillatorNode.h
 * Multi-waveform oscillator DSP node
 */

#pragma once

#include "../DSPNode.h"
#include <JuceHeader.h>

namespace vexel {

enum class OscillatorWaveform {
    Sine,
    Saw,
    Square,
    Triangle,
    Noise
};

/**
 * Audio oscillator with multiple waveforms
 */
class OscillatorNode : public DSPNode
{
public:
    OscillatorNode();
    ~OscillatorNode() override = default;

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(const float* const* input, float* const* output, int numSamples) override;
    void reset() override;

    // Parameters
    void setParameter(const juce::String& paramName, float value) override;
    float getParameter(const juce::String& paramName) const override;
    juce::StringArray getParameterNames() const override;

    // Direct setters
    void setFrequency(float frequencyHz) { m_frequency = frequencyHz; }
    void setWaveform(OscillatorWaveform waveform) { m_waveform = waveform; }
    void setAmplitude(float amplitude) { m_amplitude = juce::jlimit(0.0f, 1.0f, amplitude); }
    void setPhase(float phaseRadians) { m_phase = phaseRadians; }

    float getFrequency() const { return m_frequency; }
    OscillatorWaveform getWaveform() const { return m_waveform; }
    float getAmplitude() const { return m_amplitude; }

private:
    float m_frequency = 440.0f;
    float m_amplitude = 0.5f;
    float m_phase = 0.0f;
    float m_phaseIncrement = 0.0f;

    OscillatorWaveform m_waveform = OscillatorWaveform::Sine;

    juce::Random m_random;

    void updatePhaseIncrement();
    float generateSample();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorNode)
};

} // namespace vexel
