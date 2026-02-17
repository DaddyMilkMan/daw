/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include <stack>
#include <functional>

namespace zenith {
namespace ui {

// Action types for undo/redo
enum class ActionType {
    AudioProcessing,
    ParameterChange,
    ProjectModification,
    ModelTraining,
    SettingsChange,
    Custom
};

// Undo/redo action interface
class CustomAction : public UndoableAction {
public:
    using ExecuteFunc = std::function<bool()>;
    using UndoFunc = std::function<bool()>;
    using RedoFunc = std::function<bool()>;

    CustomAction(const juce::String& description,
                 ActionType type,
                 ExecuteFunc executeFunc,
                 UndoFunc undoFunc,
                 RedoFunc redoFunc);

    // UndoableAction overrides
    juce::String getDescription() const override { return description; }
    ActionType getType() const override { return actionType; }
    juce::Time getTimestamp() const override { return timestamp; }

    bool execute() override;
    bool undo() override;
    bool redo() override;

    size_t getMemoryUsage() const override;

private:
    juce::String description;
    ActionType actionType;
    ExecuteFunc executeFunc;
    UndoFunc undoFunc;
    RedoFunc redoFunc;
    juce::Time timestamp;
    bool hasBeenExecuted = false;
};

// Main undo/redo system

} // namespace
