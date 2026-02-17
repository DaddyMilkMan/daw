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
class RingModulator : public DSPModule {
public:
    RingModulator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setCarrierFrequency(float freq);
    void setCarrierWaveform(int wave);

    float processSample(float input);
    void processBlock(float* output, int numSamples);

private:
    float carrierPhase = 0.0f;
    float carrierFreq = 440.0f;
    int waveform = 0; // 0 = sine
    double sampleRate = 44100.0;

    float getCarrier();
};

//==============================================================================
// Filter Modules
//==============================================================================

} // namespace
