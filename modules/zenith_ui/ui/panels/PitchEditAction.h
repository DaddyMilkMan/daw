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
class PitchEditAction
{
public:
    enum class Type
    {
        MoveNote,
        MovePoint,
        SplitNote,
        MergeNotes,
        DeleteNote,
        ChangePitch,
        ChangeDrift,
        SetParameter
    };

    Type type;
    int noteIndex = -1;
    int pointIndex = -1;
    double oldTime = 0.0, newTime = 0.0;
    float oldPitch = 0.0f, newPitch = 0.0f;
    float oldValue = 0.0f, newValue = 0.0f;
    EditableNote oldNote;  // For full note operations
    EditableNote newNote;

    void undo(std::vector<EditableNote>& notes);
    void redo(std::vector<EditableNote>& notes);
};

//==============================================================================
/**
    ACTUALLY WORKING pitch curve editor.

    This is the real deal - full pitch editing like Melodyne/Auto-Tune Graph.
*/

} // namespace
