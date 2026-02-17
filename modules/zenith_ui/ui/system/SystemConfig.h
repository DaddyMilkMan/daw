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
    struct SystemConfig {
        size_t maxUndoSteps = 100;
        size_t maxMemoryUsage = 100 * 1024 * 1024;  // 100MB
        bool autoMerge = true;
        bool enableCompression = false;
        int compressionLevel = 6;
    };

    UndoRedoSystem();
    ~UndoRedoSystem();

    // Configuration
    void setConfig(const SystemConfig& config);
    SystemConfig getConfig() const { return config; }

    // Action management
    void addAction(std::unique_ptr<UndoableAction> action);
    void executeAndAdd(std::unique_ptr<UndoableAction> action);

    // Undo/redo operations
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    // Action information
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    std::vector<juce::String> getUndoHistory(int maxItems = 10) const;
    std::vector<juce::String> getRedoHistory(int maxItems = 10) const;

    // Batch operations
    void beginBatch(const juce::String& batchDescription);
    void endBatch();
    bool isInBatch() const;

    // Memory management
    void clear();
    void clearUndoHistory();
    void clearRedoHistory();
    size_t getMemoryUsage() const;
    void optimizeMemory();

    // Persistence
    bool saveHistory(const juce::File& filePath) const;
    bool loadHistory(const juce::File& filePath);

    // Action Factory
    static std::unique_ptr<UndoableAction> createActionFromVar(const juce::var& v);

    // Statistics
    int getUndoCount() const;
    int getRedoCount() const;
    size_t getTotalActions() const;
    juce::Time getOldestActionTime() const;
    juce::Time getNewestActionTime() const;

    // Listeners

} // namespace
