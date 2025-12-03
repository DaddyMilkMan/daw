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
    // The "Fake" EQ-based separation has been removed as requested.
    // Real separation requires the ONNX model integration.
    // For now, we output silence to indicate the feature is inactive/waiting for model.
    
    outputBlock.clear();
    
    // Log once per stream ideally, but DBG is safe-ish here for dev
    // DBG("DSPStemSeparator: Real separation model not loaded."); 
}

} // namespace zenith
