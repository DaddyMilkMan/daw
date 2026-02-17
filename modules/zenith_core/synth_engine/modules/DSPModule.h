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
class DSPModule {

public:
    virtual ~DSPModule() = default;
    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void reset() = 0;
};

//==============================================================================
// Oscillator Modules
//==============================================================================

} // namespace
