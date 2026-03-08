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
    struct Formant {
        float freq = 1000.0f;
        float bandwidth = 100.0f;
    };

    juce::Array<Formant> formants;

    // Each formant is a bandpass filter
    juce::Array<StateVariableFilter> filters;
    double sampleRate = 44100.0;
};

} // namespace zenith
