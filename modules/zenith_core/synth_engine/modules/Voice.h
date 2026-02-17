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
    struct Voice {
        float phase = 0.0f;
        float detune = 0.0f;
        float pan = 0.5f;
    };

    juce::Array<Voice> voices;
    int numVoices = 4;
    float detuneAmount = 5.0f;
    float spread = 0.5f;
    float mix = 0.5f;
    double sampleRate = 44100.0;
};

//==============================================================================

} // namespace
