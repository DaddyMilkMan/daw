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
class NoiseGenerator : public DSPModule {
public:
    enum Type { White, Pink };

    NoiseGenerator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setType(Type type);
    float processSample();
    void processBlock(float* output, int numSamples);

private:
    Type type = Type::White;
    juce::Random random;

    // Pink noise filter state
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
};

//==============================================================================

} // namespace
