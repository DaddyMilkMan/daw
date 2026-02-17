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
class FMOperator : public DSPModule {
public:
    FMOperator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Set frequency ratio (relative to base frequency)
    void setRatio(float ratio);

    // Set modulation index (depth of FM)
    void setIndex(float index);

    // Set feedback amount
    void setFeedback(float fb);

    // Set waveform
    void setWaveform(int wave);

    // Process with modulation input
    float processSample(float modulation);
    void processBlock(float* output, const float* modulation, int numSamples);

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    float ratio = 1.0f;
    float index = 0.0f;
    float feedback = 0.0f;
    float lastOutput = 0.0f;
    int waveform = 0; // 0 = sine

    void updatePhaseIncrement();
};

//==============================================================================

} // namespace
