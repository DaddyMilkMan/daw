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
class WavetableOscillator : public DSPModule {
public:
    WavetableOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Load wavetable from memory
    bool loadWavetable(const float* data, int numFrames);

    // Set morph position between two tables (0.0 to 1.0)
    void setMorph(float position);

    // Set pitch
    void setPitch(float semitones);

    // Process
    float processSample();
    void processBlock(float* output, int numSamples);

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    float morphPosition = 0.0f;

    // Wavetable data
    const float* wavetableA = nullptr;
    const float* wavetableB = nullptr;
    int numFrames = 256;

    void updatePhaseIncrement();
    float interpolate(float phase);
};

//==============================================================================

} // namespace
