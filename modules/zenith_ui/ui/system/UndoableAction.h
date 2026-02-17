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
class UndoableAction {
public:
    virtual ~UndoableAction() = default;

    // Action identification
    virtual juce::String getDescription() const = 0;
    virtual ActionType getType() const = 0;
    virtual juce::Time getTimestamp() const = 0;

    // Core undo/redo operations
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;

    // Action state
    virtual bool canUndo() const { return true; }
    virtual bool canRedo() const { return true; }
    virtual bool isReversible() const { return true; }

    // Memory management
    virtual size_t getMemoryUsage() const = 0;
    virtual void cleanup() {}

    // Serialization
    virtual juce::var toVar() const;

    // Merging for consecutive similar actions
    virtual bool canMergeWith(const UndoableAction& other) const { return false; }
    virtual void mergeWith(const UndoableAction& other) {}
};

// Audio processing action

} // namespace
