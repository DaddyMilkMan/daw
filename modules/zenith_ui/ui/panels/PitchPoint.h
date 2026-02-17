/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <stack>

namespace zenith {
namespace ui {

//==============================================================================
/**
    Represents a pitch point on the curve.
*/
struct PitchPoint
{
    double time = 0.0;           // Time in seconds
    float pitchSemitones = 0.0f; // Pitch relative to reference
    bool isEditable = true;      // Can user edit this point?
    bool isDriftPoint = false;   // Is this a pitch drift correction?

    // For curve interpolation
    float tension = 0.5f;        // 0=linear, 0.5=smooth, 1=step
};

//==============================================================================
/**
    A detected note with full editing capabilities.
*/

} // namespace
