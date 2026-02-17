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
class ProjectModificationAction : public UndoableAction {
public:
    enum class ModificationType {
        AddTrack,
        RemoveTrack,
        MoveTrack,
        AddEffect,
        RemoveEffect,
        ReorderEffects,
        ChangeTrackSettings
    };

    ProjectModificationAction(ModificationType type,
                             const juce::String& description,
                             const juce::var& beforeState,
                             const juce::var& afterState,
                             std::function<bool(const juce::var&)> modifier);

    // UndoableAction overrides
    juce::String getDescription() const override { return description; }
    ActionType getType() const override { return ActionType::ProjectModification; }
    juce::Time getTimestamp() const override { return timestamp; }

    bool execute() override;
    bool undo() override;
    bool redo() override;

    size_t getMemoryUsage() const override;

    // Project-specific methods
    ModificationType getModificationType() const { return modificationType; }
    const juce::var& getBeforeState() const { return beforeState; }
    const juce::var& getAfterState() const { return afterState; }

private:
    ModificationType modificationType;
    juce::String description;
    juce::var beforeState;
    juce::var afterState;
    std::function<bool(const juce::var&)> modifier;
    juce::Time timestamp;
    bool hasBeenExecuted = false;
};

// Settings change action

} // namespace
