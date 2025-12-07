/*
  ==============================================================================

    ZenithFilter.cpp
    Created: 2025-12-06
    Author:  Zenith DAW

    Implementation of ZenithFilter.

  ==============================================================================
*/

#include "ZenithFilter.h"
#include <cmath>

namespace zenith {

void ZenithFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    cutoffSmoothed_.reset(sampleRate, 0.05);
    resonanceSmoothed_.reset(sampleRate, 0.05);
}

void ZenithFilter::setCutoff(float cutoffHz) {
    cutoffSmoothed_.setTargetValue(juce::jlimit(20.0f, 20000.0f, cutoffHz));
}

void ZenithFilter::setResonance(float resonance) {
    resonanceSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, resonance));
}

void ZenithFilter::reset() {
    v0_ = v1_ = v2_ = 0.0f;
    ic1eq_ = ic2eq_ = 0.0f;
    cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
    resonanceSmoothed_.setCurrentAndTargetValue(0.0f);
}

float ZenithFilter::processSample(float input) {
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();
    
    // Apply drive
    if (drive_ > 1.0f) {
        input *= drive_;
        input = std::tanh(input);
    }
    
    // State variable filter
    float g = std::tan(juce::MathConstants<float>::pi * cutoff / static_cast<float>(sampleRate_));
    float k = 2.0f - 2.0f * resonance;
    
    // Pre-calculate to save operations
    float gk = g + k;
    float a1 = 1.0f / (1.0f + g * gk);
    float a2 = g * a1;
    float a3 = g * a2;
    
    v0_ = input;
    v1_ = a1 * ic1eq_ + a2 * (v0_ - ic2eq_);
    v2_ = ic2eq_ + a2 * ic1eq_ + a3 * (v0_ - ic2eq_);
    
    ic1eq_ = 2.0f * v1_ - ic1eq_;
    ic2eq_ = 2.0f * v2_ - ic2eq_;
    
    switch (type_) {
        case FilterType::Lowpass:
            return v2_;
        case FilterType::Bandpass:
            return v1_;
        case FilterType::Highpass:
            return v0_ - k * v1_ - v2_;
        default:
            return v2_;
    }
}

} // namespace zenith
