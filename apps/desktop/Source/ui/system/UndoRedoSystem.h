/*
  ==============================================================================
    UndoRedoSystem.h
    Comprehensive undo/redo system for audio processing
    Phase 4: User Interface
  ==============================================================================
*/

#pragma once

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
class AudioProcessingAction : public UndoableAction {
public:
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
class ParameterChangeAction : public UndoableAction {
public:
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
class SettingsChangeAction : public UndoableAction {
public:
    struct SettingState {
        juce::String settingId;
        juce::var oldValue;
        juce::var newValue;
        juce::String category;
    };
    
    SettingsChangeAction(const std::vector<SettingState>& changes);
    
    // Factory method for deserialization
    static std::unique_ptr<SettingsChangeAction> fromVar(const juce::var& v);
    
    // UndoableAction overrides
    juce::String getDescription() const override;
    ActionType getType() const override { return ActionType::SettingsChange; }
    juce::Time getTimestamp() const override { return timestamp; }
    
    bool execute() override;
    bool undo() override;
    bool redo() override;
    
    size_t getMemoryUsage() const override;
    
    // Settings-specific methods
    bool canMergeWith(const UndoableAction& other) const override;
    void mergeWith(const UndoableAction& other) override;
    
    const std::vector<SettingState>& getChanges() const { return changes; }
    
private:
    std::vector<SettingState> changes;
    juce::Time timestamp;
    bool hasBeenExecuted = false;
    
    juce::String generateDescription() const;
};

// Custom action for user-defined operations
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
class UndoRedoSystem {
public:
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
class UndoRedoManager {
public:
    UndoRedoManager();
    ~UndoRedoManager();
    
    // System access
    UndoRedoSystem& getSystem() { return *undoRedoSystem; }
    
    // GUI integration
    void setUndoButton(juce::Button* button);
    void setRedoButton(juce::Button* button);
    void setUndoMenu(juce::PopupMenu* menu);
    void setRedoMenu(juce::PopupMenu* menu);
    
    // Automatic GUI updates
    void enableAutoUpdate(bool enabled);
    bool isAutoUpdateEnabled() const;
    
    // Keyboard shortcuts
    void setUndoKey(const juce::KeyPress& key);
    void setRedoKey(const juce::KeyPress& key);
    bool handleKeyPress(const juce::KeyPress& key);
    
    // Status display
    void setStatusLabel(juce::Label* label);
    void updateStatusLabel();
    
    // Action creation helpers
    void recordAudioProcessing(const juce::String& description,
                              const juce::AudioBuffer<float>& beforeAudio,
                              const juce::AudioBuffer<float>& afterAudio,
                              double sampleRate,
                              std::function<bool(const juce::AudioBuffer<float>&)> processor);
    
    void recordParameterChange(const juce::String& parameterId,
                               const juce::var& oldValue,
                               const juce::var& newValue,
                               const juce::String& componentId = "");
    
    void recordSettingsChange(const juce::String& settingId,
                              const juce::var& oldValue,
                              const juce::var& newValue,
                              const juce::String& category = "");
    
    // Component overrides for keyboard handling
    bool keyPressed(const juce::KeyPress& key);
    
private:
    std::unique_ptr<UndoRedoSystem> undoRedoSystem;
    
    // GUI components
    juce::Button* undoButton = nullptr;
    juce::Button* redoButton = nullptr;
    juce::PopupMenu* undoMenu = nullptr;
    juce::PopupMenu* redoMenu = nullptr;
    juce::Label* statusLabel = nullptr;
    
    // Settings
    bool autoUpdateEnabled = true;
    juce::KeyPress undoKey;
    juce::KeyPress redoKey;
    
    // Update methods
    void updateUndoButton();
    void updateRedoButton();
    void updateUndoMenu();
    void updateRedoMenu();
    
    // Event handling
    void onUndoButtonClicked();
    void onRedoButtonClicked();
    void onUndoMenuItemSelected(int menuItemId);
    void onRedoMenuItemSelected(int menuItemId);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoRedoManager)
};

// Global undo/redo instance with thread-safe Meyer's singleton
class GlobalUndoRedo {
public:
    /**
     * @brief Get the global UndoRedoSystem instance
     * 
     * Thread-safe Meyer's singleton. The instance is created on first access
     * and lives for the entire program lifetime.
     */
    static UndoRedoSystem& getInstance();
    
    /**
     * @brief Get the global UndoRedoManager instance
     * 
     * Thread-safe Meyer's singleton. The instance is created on first access
     * and lives for the entire program lifetime.
     */
    static UndoRedoManager& getManager();
    
private:
    GlobalUndoRedo() = delete;
};

} // namespace ui
} // namespace zenith
