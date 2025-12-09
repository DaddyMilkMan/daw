/*
  ==============================================================================

    ZenithOscillator.cpp
    Created: 2025-12-06
    Author:  Zenith DAW

    Implementation of ZenithOscillator.

  ==============================================================================
*/

#include "ZenithOscillator.h"
#include <cmath>

namespace zenith {

// Optimization: Use std::exp2 for powers of 2 (2^x)
// std::pow(2.0f, x) is equivalent to std::exp2(x)
// For detune 2^(cents/1200) -> exp2(cents/1200)

void ZenithOscillator::setDetune(float detuneCents) {
    if (detuneCents_ != detuneCents) {
        detuneCents_ = detuneCents;
        // Optionally update supersaw ratios immediately if this is not called high-frequency
        // But getNextSample calls updateSupersawRatios check? 
        // No, current logic requires explicit call or check.
        // We will call it here to ensure it's always up to date when parameter changes.
        if (supersawInit_) {
            updateSupersawRatios();
        }
    }
}

float ZenithOscillator::getNextSample(float frequency, float shape) {
    // Apply detune
    // Optimization: std::pow(2.0f, x) -> std::exp2(x)
    float detuneMultiplier = std::exp2(detuneCents_ / 1200.0f);
    frequency *= detuneMultiplier;
    
    switch (waveform_) {
        case OscillatorWaveform::Sine:
            return processSine(frequency);
        case OscillatorWaveform::Saw:
            return processSaw(frequency);
        case OscillatorWaveform::Square:
            return processSquare(frequency, shape);
        case OscillatorWaveform::Triangle:
            return processTriangle(frequency);
        case OscillatorWaveform::Noise:
            return processNoise();
        case OscillatorWaveform::Supersaw:
            return processSupersaw(frequency);
        default:
            return 0.0f;
    }
}

void ZenithOscillator::updateSupersawRatios() {
    // Optimization: std::pow -> std::exp2
    float spread = 1.0f + (detuneCents_ / 100.0f);
    for (int i = 0; i < 7; ++i) {
        // (supersawDetunes_[i] * spread) / 12.0f gives octaves
        supersawRatios_[i] = std::exp2((supersawDetunes_[i] * spread) / 12.0f);
    }
}

float ZenithOscillator::processSine(float frequency) {
    float sample = std::sin(phase_ * juce::MathConstants<double>::twoPi);
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSaw(float frequency) {
    float phaseInc = frequency / sampleRate_;
    float sample = 2.0f * static_cast<float>(phase_) - 1.0f;
    
    // PolyBLEP
    sample -= poly_blep(static_cast<float>(phase_), phaseInc);
    
    phase_ += phaseInc;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processSquare(float frequency, float pulseWidth) {
    float phaseInc = frequency / sampleRate_;
    float sample = (phase_ < pulseWidth) ? 1.0f : -1.0f;
    
    // PolyBLEP (for both edges)
    sample += poly_blep(static_cast<float>(phase_), phaseInc);
    
    // Second edge at pulseWidth
    // We need to map phase relative to pulseWidth
    float phase2 = static_cast<float>(phase_) - pulseWidth;
    if (phase2 < 0.0f) phase2 += 1.0f;
    sample -= poly_blep(phase2, phaseInc);

    phase_ += phaseInc;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processTriangle(float frequency) {
    float sample;
    if (phase_ < 0.5) {
        sample = 4.0f * static_cast<float>(phase_) - 1.0f;
    } else {
        sample = 3.0f - 4.0f * static_cast<float>(phase_);
    }
    phase_ += frequency / sampleRate_;
    if (phase_ >= 1.0) phase_ -= 1.0;
    return sample;
}

float ZenithOscillator::processNoise() {
    return random_.nextFloat() * 2.0f - 1.0f;
}

float ZenithOscillator::processSupersaw(float frequency) {
    if (!supersawInit_) {
        supersawDetunes_[0] = 0.0f;
        supersawDetunes_[1] = -0.11f; supersawDetunes_[2] = 0.11f;
        supersawDetunes_[3] = -0.06f; supersawDetunes_[4] = 0.06f;
        supersawDetunes_[5] = -0.02f; supersawDetunes_[6] = 0.02f;
        
        for (auto& phase : supersawPhases_) phase = random_.nextFloat();
        supersawInit_ = true;
        updateSupersawRatios();
    }

    float sample = 0.0f;

    // Optimization: Unroll loop slightly or just trust compiler
    for (int i = 0; i < 7; ++i) {
        // Use precalculated ratios
        float detunedFreq = frequency * supersawRatios_[i];
        
        float phaseInc = detunedFreq / sampleRate_;
        
        float s = 2.0f * static_cast<float>(supersawPhases_[i]) - 1.0f;
        s -= poly_blep(static_cast<float>(supersawPhases_[i]), phaseInc);
        
        sample += s;
        
        supersawPhases_[i] += phaseInc;
        if (supersawPhases_[i] >= 1.0) supersawPhases_[i] -= 1.0;
    }

    return sample * 0.15f; 
}

} // namespace zenith
