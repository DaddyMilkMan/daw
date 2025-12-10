/*
  ==============================================================================

    ZenithFilter.cpp
    Created: 2025-12-06
    Refactored: 2025-12-09 (Flagship Update)
    Author:  Zenith DAW

    Implementation of ZenithFilter with SVF and Moog Ladder models.

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
    ic1eq_ = ic2eq_ = 0.0f;
    l_z1 = l_z2 = l_z3 = l_z4 = 0.0;
    cutoffSmoothed_.setCurrentAndTargetValue(1000.0f);
    resonanceSmoothed_.setCurrentAndTargetValue(0.0f);
}

float ZenithFilter::processSample(float input) {
    float cutoff = cutoffSmoothed_.getNextValue();
    float resonance = resonanceSmoothed_.getNextValue();
    
    // Apply pre-filter drive (common for both models)
    // Tanh drive for warmth if drive > 1.0
    if (drive_ > 1.0f) {
        input *= drive_;
        input = std::tanh(input); // Soft clipper
    }
    
    if (model_ == 1) {
        return processLadder(input);
    } else {
        return processSVF(input);
    }
}

float ZenithFilter::processSVF(float input) {
    // Re-calculating coefs per sample is expensive but allows audio-rate modulation.
    // Optimization: In a real "Pro" synth, we might update these block-wise or use an approximation.
    // But for 100% accuracy we do it per sample.

    float cutoff = cutoffSmoothed_.getCurrentValue();
    float resonance = resonanceSmoothed_.getCurrentValue();
    
    float g = std::tan(juce::MathConstants<float>::pi * cutoff / static_cast<float>(sampleRate_));
    float k = 2.0f - 2.0f * resonance;
    
    float gk = g + k;
    float a1 = 1.0f / (1.0f + g * gk);
    float a2 = g * a1;
    float a3 = g * a2;
    
    float v0 = input;
    float v1 = a1 * ic1eq_ + a2 * (v0 - ic2eq_);
    float v2 = ic2eq_ + a2 * ic1eq_ + a3 * (v0 - ic2eq_);
    
    ic1eq_ = 2.0f * v1 - ic1eq_;
    ic2eq_ = 2.0f * v2 - ic2eq_;
    
    switch (type_) {
        case FilterType::Lowpass:  return v2;
        case FilterType::Bandpass: return v1;
        case FilterType::Highpass: return v0 - k * v1 - v2;
        default: return v2;
    }
}

float ZenithFilter::processLadder(float input) {
    // Zero-Delay Feedback Moog Ladder Filter (Approximation)
    // Based on Huovilainen / Stilson topology with nonlinearities.
    
    float cutoff = cutoffSmoothed_.getCurrentValue();
    float resonance = resonanceSmoothed_.getCurrentValue();

    double cutoffRad = 2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate_;
    double f = cutoffRad * (1.0 - 0.2 * cutoffRad); // First order tuning correction
    f = juce::jlimit(0.001, 0.9, f);
    
    // Resonance compensation (Moog loses bass with resonance, we can compensate or keep authentic)
    // Authentic Moog behavior: Bass drop on high Res.
    // Let's keep it authentic but provide a simpler gain scaling.
    double k = 4.0 * resonance * (1.0 - 0.5 * f); // Empirical scaling
    
    // 4-stage ladder
    // Using a simpler topology for stability without 4x oversampling
    
    double inputVal = static_cast<double>(input);
    
    // Nonlinear feedback loop
    double feedback = l_z4;
    double driveSignal = inputVal - k * feedback;
    
    // Stage 1
    l_z1 += f * (std::tanh(driveSignal) - std::tanh(l_z1));
    // Stage 2
    l_z2 += f * (std::tanh(l_z1) - std::tanh(l_z2));
    // Stage 3
    l_z3 += f * (std::tanh(l_z2) - std::tanh(l_z3));
    // Stage 4
    l_z4 += f * (std::tanh(l_z3) - std::tanh(l_z4));
    
    return static_cast<float>(l_z4);
    
    // Note: This 4-pole is always Lowpass.
    // If user selected Bandpass/Highpass, we should technically implement those topologies for Ladder too,
    // or just fallback to SVF.
    // For now, Ladder is strictly Lowpass (Classic Moog).
}

} // namespace zenith
