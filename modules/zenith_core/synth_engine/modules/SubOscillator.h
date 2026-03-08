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
class SubOscillator : public DSPModule {
public:
    enum Waveform { Square, Sine };

    SubOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setWaveform(Waveform wave);
    void setLevel(float level);

    // Process (always -1 octave from input frequency)
    float processSample(float inputPhase);
    void processBlock(float* output, int numSamples);

private:
    Waveform waveform = Waveform::Square;
    float level = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

} // namespace
