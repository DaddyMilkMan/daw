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
struct EditableNote
{
    double startTime = 0.0;
    double endTime = 0.0;
    float detectedPitchHz = 0.0f;
    float correctedPitchHz = 0.0f;
    int midiNote = 0;

    // Pitch drift within note
    std::vector<PitchPoint> driftCurve;

    // Per-note parameters
    float correctionAmount = 1.0f;
    float formantShift = 0.0f;
    float gain = 1.0f;
    bool isBypassed = false;
    bool isSelected = false;

    // Visual
    juce::Colour color = juce::Colours::cyan;
};

//==============================================================================
/**
    Undo action for pitch editing.
*/

} // namespace
