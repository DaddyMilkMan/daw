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
class ModulationEffect {
public:
    ModulationEffect();
    ~ModulationEffect() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameters
    void setType(ModulationType type) { type_ = type; }
    void setRate(float rateHz) { rate_ = juce::jlimit(0.01f, 20.0f, rateHz); }
    void setDepth(float depth) { depth_ = juce::jlimit(0.0f, 1.0f, depth); }
    void setFeedback(float feedback) { feedback_ = juce::jlimit(-1.0f, 1.0f, feedback); }
    void setVoices(int voices) { numVoices_ = juce::jlimit(1, 8, voices); }
    void setSpread(float spread) { spread_ = juce::jlimit(0.0f, 1.0f, spread); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }

private:
    void updateLFO();
    void processChorus(juce::AudioBuffer<float>& buffer);
    void processPhaser(juce::AudioBuffer<float>& buffer);
    void processFlanger(juce::AudioBuffer<float>& buffer);

    // Chorus voices
    static constexpr int maxVoices = 8;
    std::array<std::unique_ptr<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>>, maxVoices> chorusDelays_;
    std::array<float, maxVoices> lfoPhases_;

    // Phaser allpass filters (6 stages)
    static constexpr int numPhaserStages = 6;
    std::array<std::unique_ptr<juce::dsp::FirstOrderTPTFilter<float>>, numPhaserStages> phaserStages_;

    // Flanger delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> flangerDelay_;

    // LFOs
    juce::dsp::Oscillator<float> lfo_;
    juce::dsp::Oscillator<float> lfoSlow_;

    // Parameters
    ModulationType type_ = ModulationType::Chorus;
    float rate_ = 0.5f;      // Hz
    float depth_ = 0.5f;
    float feedback_ = 0.5f;
    int numVoices_ = 4;
    float spread_ = 0.5f;
    float mix_ = 0.5f;

    // State
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> processBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationEffect)
};

//==============================================================================
// VOCODER (Band-Limited)
//==============================================================================

/**
 * @class Vocoder
 * @brief High-quality band-limited vocoder (Sennheiser VSM-201 style)
 *
 * Features:
 * - 16 band analyzers (30Hz - 16kHz)
 * - Log-spaced frequency bands (musical)
 * - Adjustable envelope follower response
 * - Formant shifting
 * - Carrier/modulator swap
 */

} // namespace
