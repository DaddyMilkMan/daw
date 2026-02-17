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
    struct Listener {
        virtual ~Listener() = default;
        virtual void actionAdded(const UndoableAction& action) {}
        virtual void actionUndone(const UndoableAction& action) {}
        virtual void actionRedone(const UndoableAction& action) {}
        virtual void historyCleared() {}
        virtual void memoryOptimized() {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Factory methods for common actions
    static std::unique_ptr<AudioProcessingAction> createAudioAction(
        const juce::String& description,
        const AudioProcessingAction::AudioState& before,
        const AudioProcessingAction::AudioState& after,
        std::function<bool(const AudioProcessingAction::AudioState&)> processor);

    static std::unique_ptr<ParameterChangeAction> createParameterAction(
        const juce::String& parameterId,
        const juce::var& oldValue,
        const juce::var& newValue,
        const juce::String& componentId = "");

    static std::unique_ptr<SettingsChangeAction> createSettingsAction(
        const juce::String& settingId,
        const juce::var& oldValue,
        const juce::var& newValue,
        const juce::String& category = "");

private:
    SystemConfig config;

    // Action storage
    std::vector<std::unique_ptr<UndoableAction>> undoStack;
    std::vector<std::unique_ptr<UndoableAction>> redoStack;

    // Batch operations
    std::unique_ptr<juce::String> currentBatchDescription;
    std::vector<std::unique_ptr<UndoableAction>> batchActions;

    // Listeners
    std::vector<Listener*> listeners;

    // Memory tracking
    mutable std::atomic<size_t> currentMemoryUsage{0};

    // Internal methods
    void addToUndoStack(std::unique_ptr<UndoableAction> action);
    void addToRedoStack(std::unique_ptr<UndoableAction> action);
    void optimizeMemoryUsage();
    bool shouldMergeActions(const UndoableAction& newer, const UndoableAction& older) const;
    void mergeActions(std::unique_ptr<UndoableAction>& newer, std::unique_ptr<UndoableAction>& older);
    void enforceMemoryLimits();
    void calculateMemoryUsage() const;

    // Notification
    void notifyActionAdded(const UndoableAction& action);
    void notifyActionUndone(const UndoableAction& action);
    void notifyActionRedone(const UndoableAction& action);
    void notifyHistoryCleared();
    void notifyMemoryOptimized();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoRedoSystem)
};

// Undo/redo manager with GUI integration

} // namespace
