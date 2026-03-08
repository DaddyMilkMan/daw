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
    struct Partial {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float level = 0.0f;
        float ratio = 1.0f;
    };

    juce::Array<Partial> partials;
    double sampleRate = 44100.0;
    float baseFreq = 440.0f;

    void updatePhaseIncrements(float baseFreq);
};

//==============================================================================

} // namespace
