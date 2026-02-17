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
    struct ParameterState {
        juce::String parameterId;
        juce::var oldValue;
        juce::var newValue;
        juce::String componentId;
    };

    ParameterChangeAction(const std::vector<ParameterState>& changes);

    // Factory method for deserialization
    static std::unique_ptr<ParameterChangeAction> fromVar(const juce::var& v);

    // UndoableAction overrides
    juce::String getDescription() const override;
    ActionType getType() const override { return ActionType::ParameterChange; }
    juce::Time getTimestamp() const override { return timestamp; }

    bool execute() override;
    bool undo() override;
    bool redo() override;

    size_t getMemoryUsage() const override;

    // Parameter-specific methods
    bool canMergeWith(const UndoableAction& other) const override;
    void mergeWith(const UndoableAction& other) override;

    const std::vector<ParameterState>& getChanges() const { return changes; }

private:
    std::vector<ParameterState> changes;
    juce::Time timestamp;
    bool hasBeenExecuted = false;

    juce::String generateDescription() const;
};

// Project modification action

} // namespace
