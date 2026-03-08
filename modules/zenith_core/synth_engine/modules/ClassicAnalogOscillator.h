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
class ClassicAnalogOscillator : public DSPModule {
public:
    enum Waveform { Sine, Saw, Square, Triangle };

    ClassicAnalogOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Set waveform
    void setWaveform(Waveform wave);

    // Set pitch (in semitones relative to A4)
    void setPitch(float semitones);

    // Set detune (in cents, -100 to +100)
    void setDetune(float cents);

    // Set pulse width for square wave (0.0 to 1.0)
    void setPulseWidth(float pw);

    // Process one sample
    float processSample();

    // Process block (SIMD optimized)
    void processBlock(float* output, int numSamples);

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double sampleRate = 44100.0;
    Waveform waveform = Waveform::Saw;
    float pitchSemitones = 0.0f;
    float detuneCents = 0.0f;
    float pulseWidth = 0.5f;

    // Bandlimiting tables
    static constexpr int BLEP_SIZE = 1024;
    static float blepTable[BLEP_SIZE];

    void updatePhaseIncrement();
    float blep(float phase, float step);
};

//==============================================================================

} // namespace
