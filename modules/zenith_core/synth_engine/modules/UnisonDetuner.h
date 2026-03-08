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
class UnisonDetuner : public DSPModule {
public:
    UnisonDetuner();
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    void setNumVoices(int voices);
    void setDetune(float cents);
    void setSpread(float amount); // Stereo spread
    void setMix(float mix); // 0.0 = original, 1.0 = full unison

    // Process (modifies input buffer)
    void processBlock(float* left, float* right, int numSamples);

private:

} // namespace
