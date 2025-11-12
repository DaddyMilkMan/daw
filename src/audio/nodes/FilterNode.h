/**
 * FilterNode.h
 * Multi-mode filter (lowpass, highpass, bandpass, notch)
 */

#pragma once

#include "../DSPNode.h"
#include <JuceHeader.h>

namespace vexel {

enum class FilterMode {
    LowPass,
    HighPass,
    BandPass,
    Notch
};

/**
 * State variable filter with multiple modes
 */
class FilterNode : public DSPNode
{
public:
    FilterNode();
    ~FilterNode() override = default;

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(const float* const* input, float* const* output, int numSamples) override;
    void reset() override;

    // Parameters
    void setParameter(const juce::String& paramName, float value) override;
    float getParameter(const juce::String& paramName) const override;
    juce::StringArray getParameterNames() const override;

    // Direct setters
    void setCutoffFrequency(float frequencyHz);
    void setResonance(float resonance);  // 0.0 - 1.0
    void setFilterMode(FilterMode mode);

    float getCutoffFrequency() const { return m_cutoffFrequency; }
    float getResonance() const { return m_resonance; }
    FilterMode getFilterMode() const { return m_mode; }

private:
    float m_cutoffFrequency = 1000.0f;
    float m_resonance = 0.7f;
    FilterMode m_mode = FilterMode::LowPass;

    // State variable filter state (per channel)
    struct ChannelState {
        float low = 0.0f;
        float band = 0.0f;
        float high = 0.0f;
    };

    std::vector<ChannelState> m_channelStates;

    float m_f = 0.0f;  // Frequency parameter
    float m_q = 0.0f;  // Resonance parameter

    void updateCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterNode)
};

} // namespace vexel
