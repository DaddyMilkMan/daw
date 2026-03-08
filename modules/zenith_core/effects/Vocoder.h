/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// ZenithAdvancedEffects.h

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

namespace zenith {

//==============================================================================
// ALGORITHMIC REVERB (Moogerfooger MF-104M Style)
//==============================================================================

/**
 * @class AlgorithmicReverb
 * @brief High-quality algorithmic reverb inspired by Moogerfooger MF-104M
 *
 * Features:
 * - 8 parallel delay lines for lush decay
 * - Diffusion network for smooth reverb tail
 * - 3-band decay time control
 * - Modulated delay times for richness
 * - Low/high frequency damping
 */
class Vocoder {
public:
    Vocoder();
    ~Vocoder() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& modulator,
                 juce::AudioBuffer<float>& carrier,
                 juce::AudioBuffer<float>& output);

    // Parameters
    void setNumBands(int bands) { numBands_ = juce::jlimit(8, 32, bands); updateBands(); }
    void setAttack(float attackMs) { attackMs_ = juce::jlimit(0.1f, 100.0f, attackMs); updateEnvelope(); }
    void setRelease(float releaseMs) { releaseMs_ = juce::jlimit(10.0f, 1000.0f, releaseMs); updateEnvelope(); }
    void setFormantShift(float shift) { formantShift_ = juce::jlimit(-12.0f, 12.0f, shift); updateBands(); }
    void setQFactor(float q) { q_ = juce::jlimit(1.0f, 20.0f, q); updateBands(); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setGate(float threshold) { gateThreshold_ = juce::jlimit(-60.0f, 0.0f, threshold); }

private:
    void updateBands();
    void updateEnvelope();

    // Band filters
    static constexpr int maxBands = 32;
    std::array<std::unique_ptr<juce::dsp::IIR::Filter<float>>, maxBands * 2> bandFilters_;  // modulator + carrier
    std::array<std::unique_ptr<juce::dsp::IIR::Filter<float>>, maxBands> envelopeFollowers_;

    // Bandpass frequencies (log-spaced)
    std::array<double, maxBands> centerFrequencies_;
    std::array<float, maxBands> bandEnvelopes_;

    // Smoothing
    std::array<float, maxBands> smoothedEnvelopes_;

    // Parameters
    int numBands_ = 16;
    float attackMs_ = 5.0f;
    float releaseMs_ = 100.0f;
    float formantShift_ = 0.0f;
    float q_ = 4.0f;
    float mix_ = 1.0f;
    float gateThreshold_ = -60.0f;

    // State
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> analysisBuffer_;
    juce::AudioBuffer<float> synthesisBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vocoder)
};

} // namespace zenith
