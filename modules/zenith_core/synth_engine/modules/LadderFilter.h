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
class LadderFilter : public DSPModule {
public:
    LadderFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setCutoff(float freqHz);
    void setResonance(float res); // 0.0 to 1.0
    void setDrive(float drive); // Soft clipping

    float processSample(float input);
    void processBlock(float* output, int numSamples);

private:
    float cutoff = 1000.0f;
    float resonance = 0.5f;
    float drive = 0.0f;
    double sampleRate = 44100.0;

    // 4-pole ladder state
    float z0 = 0.0f, z1 = 0.0f, z2 = 0.0f, z3 = 0.0f, z4 = 0.0f;

    void updateCoefficients();
};

//==============================================================================

} // namespace
