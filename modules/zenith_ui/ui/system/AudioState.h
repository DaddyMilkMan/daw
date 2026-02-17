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
    struct AudioState {
        juce::AudioBuffer<float> audioBuffer;
        double sampleRate;
        juce::String processingChain;
        juce::var parameters;
    };

    AudioProcessingAction(const juce::String& description,
                          const AudioState& beforeState,
                          const AudioState& afterState,
                          std::function<bool(const AudioState&)> processor);

    // UndoableAction overrides
    juce::String getDescription() const override { return description; }
    ActionType getType() const override { return ActionType::AudioProcessing; }
    juce::Time getTimestamp() const override { return timestamp; }

    bool execute() override;
    bool undo() override;
    bool redo() override;

    size_t getMemoryUsage() const override;

    // Audio-specific methods
    const AudioState& getBeforeState() const { return beforeState; }
    const AudioState& getAfterState() const { return afterState; }

private:
    juce::String description;
    AudioState beforeState;
    AudioState afterState;
    std::function<bool(const AudioState&)> processor;
    juce::Time timestamp;
    bool hasBeenExecuted = false;
};

// Parameter change action

} // namespace
