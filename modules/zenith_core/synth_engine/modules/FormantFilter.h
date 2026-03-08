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
class FormantFilter : public DSPModule {
public:
    FormantFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Vowel presets
    enum Vowel { A, E, I, O, U };
    void setVowel(Vowel vowel);

    // Custom formants (up to 4)
    void setFormant(int index, float freqHz, float bandwidth);

    float processSample(float input);
    void processBlock(float* output, int numSamples);

private:

} // namespace
