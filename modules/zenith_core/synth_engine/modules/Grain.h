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
    struct Grain {
        bool active = false;
        float position = 0.0f; // In sample frames
        float age = 0.0f; // In seconds
        float duration = 0.1f; // In seconds
        float pitchOffset = 0.0f;
        float pan = 0.5f;
        float amplitude = 1.0f;
    };

    juce::Array<Grain> grains;
    const float* sampleData = nullptr;
    int sampleLength = 0;
    double sampleRate = 44100.0;

    float grainSize = 0.1f;
    float grainDensity = 10.0f;
    float position = 0.0f;
    float pitch = 0.0f;
    float randomness = 0.0f;

    float accumulatedDensity = 0.0f;

    void triggerGrain();
    float processGrain(Grain& grain);
};

//==============================================================================

} // namespace
