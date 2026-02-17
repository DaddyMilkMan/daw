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
class GranularEngine : public DSPModule {
public:
    static constexpr int MAX_GRAINS = 16;

    GranularEngine();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    // Load sample buffer
    void setSample(const float* data, int numSamples);

    // Grain parameters
    void setGrainSize(float sizeSeconds);
    void setGrainDensity(float grainsPerSecond);
    void setPosition(float position); // 0.0 to 1.0
    void setPitch(float semitones);
    void setRandomness(float amount); // 0.0 to 1.0

    float processSample();
    void processBlock(float* output, int numSamples);

private:

} // namespace
