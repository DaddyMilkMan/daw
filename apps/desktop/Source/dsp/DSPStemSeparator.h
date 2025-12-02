/*
  ==============================================================================

    DSPStemSeparator.h
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team

    Real-time stem separation using Mid-Side processing and Linkwitz-Riley filters.
    This is a lightweight, zero-latency alternative to heavy AI models.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class DSPStemSeparator {
public:
    enum class StemType {
        Vocals, // Mid channel > 200Hz
        Bass,   // Mid channel < 200Hz
        Drums,  // Transient heavy (simulated via EQ)
        Other   // Side channel (Stereo width)
    };

    DSPStemSeparator();
    ~DSPStemSeparator();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    /**
     * @brief Processes a block of audio and extracts the requested stem.
     * @param inputBlock Input audio buffer
     * @param outputBlock Output audio buffer (will be overwritten)
     * @param stemType The type of stem to extract
     */
    void process(const juce::dsp::AudioBlock<const float>& inputBlock,
                 juce::dsp::AudioBlock<float>& outputBlock,
                 StemType stemType);

private:
    // Filter chains for crossover
    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>; // 4th order

    FilterChain lowPassFilter_;
    FilterChain highPassFilter_;

    double sampleRate_ = 44100.0;
    
    // Helper to calculate Mid-Side
    void computeMidSide(const float* left, const float* right, int numSamples, 
                       std::vector<float>& mid, std::vector<float>& side);
};

} // namespace zenith
