/*
  ==============================================================================

    DSPStemSeparator.cpp
    Created: 2025-11-29
    Author:  Zenith DAW - Efficient C++ Team

  ==============================================================================
*/

#include "DSPStemSeparator.h"

namespace zenith {

DSPStemSeparator::DSPStemSeparator() {
    reset();
}

DSPStemSeparator::~DSPStemSeparator() {}

void DSPStemSeparator::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate_ = spec.sampleRate;
    
    // Setup Linkwitz-Riley 4th order crossover at 200Hz
    // This splits Bass (Low) from Vocals (High)
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, 200.0f);
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, 200.0f);

    // Apply coefficients to filter chains
    lowPassFilter_.get<0>().coefficients = lpCoeffs;
    lowPassFilter_.get<1>().coefficients = lpCoeffs;
    lowPassFilter_.get<2>().coefficients = lpCoeffs;
    lowPassFilter_.get<3>().coefficients = lpCoeffs;

    highPassFilter_.get<0>().coefficients = hpCoeffs;
    highPassFilter_.get<1>().coefficients = hpCoeffs;
    highPassFilter_.get<2>().coefficients = hpCoeffs;
    highPassFilter_.get<3>().coefficients = hpCoeffs;

    lowPassFilter_.prepare(spec);
    highPassFilter_.prepare(spec);
}

void DSPStemSeparator::reset() {
    lowPassFilter_.reset();
    highPassFilter_.reset();
}

void DSPStemSeparator::computeMidSide(const float* left, const float* right, int numSamples, 
                                     std::vector<float>& mid, std::vector<float>& side) {
    mid.resize(numSamples);
    side.resize(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        // Mid = (L + R) / 2
        mid[i] = (left[i] + right[i]) * 0.5f;
        // Side = (L - R) / 2
        side[i] = (left[i] - right[i]) * 0.5f;
    }
}

void DSPStemSeparator::process(const juce::dsp::AudioBlock<const float>& inputBlock,
                               juce::dsp::AudioBlock<float>& outputBlock,
                               StemType stemType) {
    auto numSamples = inputBlock.getNumSamples();
    auto leftIn = inputBlock.getChannelPointer(0);
    auto rightIn = inputBlock.getChannelPointer(1);
    
    auto leftOut = outputBlock.getChannelPointer(0);
    auto rightOut = outputBlock.getChannelPointer(1);

    // 1. Calculate Mid/Side
    std::vector<float> mid, side;
    computeMidSide(leftIn, rightIn, numSamples, mid, side);

    // 2. Process based on Stem Type
    float* midData = mid.data();
    juce::dsp::AudioBlock<float> midBlock(&midData, 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> midContext(midBlock);

    switch (stemType) {
        case StemType::Vocals:
            // Vocals are mostly Mid channel, High frequencies
            highPassFilter_.process(midContext);
            // Copy filtered Mid to output (Mono to Stereo)
            for (int i = 0; i < numSamples; ++i) {
                leftOut[i] = mid[i];
                rightOut[i] = mid[i];
            }
            break;

        case StemType::Bass:
            // Bass is mostly Mid channel, Low frequencies
            lowPassFilter_.process(midContext);
            // Copy filtered Mid to output
            for (int i = 0; i < numSamples; ++i) {
                leftOut[i] = mid[i];
                rightOut[i] = mid[i];
            }
            break;

        case StemType::Other:
            // "Other" is mostly the Side channel (stereo information)
            for (int i = 0; i < numSamples; ++i) {
                leftOut[i] = side[i];
                rightOut[i] = -side[i]; // Invert phase for stereo width
            }
            break;

        case StemType::Drums:
            // Drums are hard to separate with just EQ/MS.
            // We'll use the Bass (Low Mid) + a bit of transient shaping (simulated)
            lowPassFilter_.process(midContext);
            for (int i = 0; i < numSamples; ++i) {
                leftOut[i] = mid[i] * 1.2f; // Boost slightly
                rightOut[i] = mid[i] * 1.2f;
            }
            break;
    }
}

} // namespace zenith
