/*
  ==============================================================================

    DSPVoiceChanger.h
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team

    Real-time voice changing using Pitch Shifting and Formant Filtering.
    This provides "Voice Cloning" style effects without AI latency.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class DSPVoiceChanger {
public:
    enum class VoiceCharacter {
        DeepMale,
        Chipmunk,
        Robot,
        Ethereal
    };

    DSPVoiceChanger();
    ~DSPVoiceChanger();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void process(const juce::dsp::AudioBlock<const float>& inputBlock,
                 juce::dsp::AudioBlock<float>& outputBlock,
                 VoiceCharacter character);

private:
    double sampleRate_ = 44100.0;
    
    // Simple delay-based pitch shifter (granular simulation)
    std::vector<float> delayBuffer_;
    int writePos_ = 0;
    float readPos_ = 0.0f;
    
    // Ring modulator for Robot effect
    double ringModPhase_ = 0.0;
    
    // Filters for Formant shaping
    juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>> formantFilters_;
};

} // namespace zenith
