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
class CombFilter : public DSPModule {
public:
    CombFilter();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setDelayTime(float timeSeconds);
    void setFeedback(float feedback); // -1.0 to 1.0
    void setBlend(float blend); // 0.0 = dry, 1.0 = wet

    float processSample(float input);
    void processBlock(float* output, int numSamples);

private:
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    int delayInSamples = 0;
    float feedback = 0.5f;
    float blend = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

} // namespace
