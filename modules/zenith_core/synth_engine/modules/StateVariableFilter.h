/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

//==============================================================================
// Base class for all DSP modules
class StateVariableFilter : public DSPModule {
public:
    enum Type { Lowpass, Highpass, Bandpass, Notch };

    StateVariableFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setType(Type type);
    void setCutoff(float freqHz);
    void setResonance(float res); // 0.0 to 1.0 (self-oscillation near 1.0)
    void setSlope(int slope); // 12 or 24 dB/oct

    float processSample(float input);
    void processBlock(float* output, int numSamples);

private:
    Type type = Type::Lowpass;
    float cutoff = 1000.0f;
    float resonance = 0.5f;
    int slope = 12; // dB/oct
    double sampleRate = 44100.0;

    // Two SVF stages for 24dB slope
    float lpf1 = 0.0f, hpf1 = 0.0f, bpf1 = 0.0f;
    float lpf2 = 0.0f, hpf2 = 0.0f, bpf2 = 0.0f;

    void updateCoefficients();
};

//==============================================================================

} // namespace
