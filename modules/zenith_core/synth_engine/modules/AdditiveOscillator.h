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
class AdditiveOscillator : public DSPModule {
public:
    static constexpr int MAX_PARTIALS = 64;

    AdditiveOscillator();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Set partial level (0.0 to 1.0)
    void setPartialLevel(int partial, float level);

    // Set partial ratio (harmonic or inharmonic)
    void setPartialRatio(int partial, float ratio);

    // Process
    float processSample();
    void processBlock(float* output, int numSamples);

private:

} // namespace
