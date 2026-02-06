/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "../../Settings.h"
#include <algorithm>
#include <random>

namespace zenith {
namespace ui {

// AudioProcessingAction Implementation
AudioProcessingAction::AudioProcessingAction(const juce::String& description,
                                           const AudioState& beforeState,
                                           const AudioState& afterState,
                                           std::function<bool(const AudioState&)> processor)
    : description(description), beforeState(beforeState), afterState(afterState), 
      processor(processor), timestamp(juce::Time::getCurrentTime()) {
}

bool AudioProcessingAction::execute() {
    if (hasBeenExecuted) {
        return redo();
    }
    
    hasBeenExecuted = true;
    return processor(afterState);
}

bool AudioProcessingAction::undo() {
    if (!hasBeenExecuted || !isReversible) {
        return false;
    }
    
    return processor(beforeState);
}

bool AudioProcessingAction::redo() {
    if (!hasBeenExecuted || !isReversible) {
        return false;
    }
    
    return processor(afterState);
}

size_t AudioProcessingAction::getMemoryUsage() const {
    size_t size = description.length() * sizeof(juce::String::CharPointerType);
    size += beforeState.audioBuffer.getNumChannels() * 
            beforeState.audioBuffer.getNumSamples() * sizeof(float);
    size += afterState.audioBuffer.getNumChannels() * 
            afterState.audioBuffer.getNumSamples() * sizeof(float);
    return size;
}

juce::var UndoableAction::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("description", getDescription());
    obj->setProperty("type", static_cast<int>(getType()));
    obj->setProperty("timestamp", getTimestamp().toMilliseconds());
    return juce::var(obj);
}

// ParameterChangeAction Implementation
ParameterChangeAction::ParameterChangeAction(const std::vector<ParameterState>& changes)
    : changes(changes), timestamp(juce::Time::getCurrentTime()) {
}

juce::String ParameterChangeAction::getDescription() const {
    if (changes.empty()) return "Parameter Change";
    
    if (changes.size() == 1) {
        return "Changed " + changes[0].parameterId;
    }
    
    return "Changed " + juce::String(changes.size()) + " parameters";
}

bool ParameterChangeAction::execute() {
    if (hasBeenExecuted) {
        return redo();
    }
    
    hasBeenExecuted = true;
    return true; // Assume initial change already happened
}

bool ParameterChangeAction::undo() {
    if (!hasBeenExecuted) return false;
    
    auto* engine = Engine::getInstance();
    if (!engine) return false;
    
    auto* projectState = engine->getProjectState();
    if (!projectState) return false;
    
    // Restore old values
    for (const auto& change : changes) {
        projectState->setProperty(change.componentId, change.parameterId, change.oldValue);
    }
    
    return true;
}

bool ParameterChangeAction::redo() {
    if (!hasBeenExecuted) return false;
    
    auto* engine = Engine::getInstance();
    if (!engine) return false;
    
    auto* projectState = engine->getProjectState();
    if (!projectState) return false;
    
    // Apply new values
    for (const auto& change : changes) {
        projectState->setProperty(change.componentId, change.parameterId, change.newValue);
    }
    
    return true;
}

size_t ParameterChangeAction::getMemoryUsage() const {
    size_t size = sizeof(*this);
    for (const auto& change : changes) {
        size += change.parameterId.length() * sizeof(juce::String::CharPointerType);
        size += change.componentId.length() * sizeof(juce::String::CharPointerType);
    }
    return size;
}

juce::var ParameterChangeAction::toVar() const {
    auto v = UndoableAction::toVar();
    auto* obj = v.getDynamicObject();
    
    juce::Array<juce::var> changesArray;
    for (const auto& change : changes) {
        juce::DynamicObject::Ptr c = new juce::DynamicObject();
        c->setProperty("parameterId", change.parameterId);
        c->setProperty("oldValue", change.oldValue);
        c->setProperty("newValue", change.newValue);
        c->setProperty("componentId", change.componentId);
        changesArray.add(juce::var(c));
    }
    obj->setProperty("changes", changesArray);
    
    return v;
}

std::unique_ptr<ParameterChangeAction> ParameterChangeAction::fromVar(const juce::var& v) {
    auto* obj = v.getDynamicObject();
    if (!obj) return nullptr;
    
    std::vector<ParameterState> changes;
    auto changesArray = obj->getProperty("changes", juce::var());
    if (changesArray.isArray()) {
        for (int i = 0; i < changesArray.size(); ++i) {
            auto* c = changesArray[i].getDynamicObject();
            if (c) {
                changes.push_back({
                    c->getProperty("parameterId").toString(),
                    c->getProperty("oldValue"),
                    c->getProperty("newValue"),
                    c->getProperty("componentId").toString()
                });
            }
        }
    }
    
    auto action = std::make_unique<ParameterChangeAction>(changes);
    action->hasBeenExecuted = true;
    return action;
}

bool ParameterChangeAction::canMergeWith(const UndoableAction& other) const {
    if (other.getType() != ActionType::ParameterChange) {
        return false;
    }
    
    const auto& otherParam = static_cast<const ParameterChangeAction&>(other);
    
    // Can merge if same parameters and within time window
    if (std::abs((timestamp - otherParam.timestamp).inMilliseconds()) > 1000) {
        return false;
    }
    
    // Check if any parameters overlap
    for (const auto& change1 : changes) {
        for (const auto& change2 : otherParam.changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) {
                return true;
            }
        }
    }
    
    return false;
}

void ParameterChangeAction::mergeWith(const UndoableAction& other) {
    const auto& otherParam = static_cast<const ParameterChangeAction&>(other);
    
    // Update with new values
    for (auto& change1 : changes) {
        for (const auto& change2 : otherParam.changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) {
                change1.newValue = change2.newValue;
            }
        }
    }
    
    // Add new parameters
    for (const auto& change2 : otherParam.changes) {
        bool found = false;
        for (const auto& change1 : changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) {
                found = true;
                break;
            }
        }
        if (!found) {
            changes.push_back(change2);
        }
    }
}

juce::String ParameterChangeAction::generateDescription() const {
    return getDescription();
}

// ProjectModificationAction Implementation
ProjectModificationAction::ProjectModificationAction(ModificationType type,
                                                   const juce::String& description,
                                                   const juce::var& beforeState,
                                                   const juce::var& afterState,
                                                   std::function<bool(const juce::var&)> modifier)
    : modificationType(type), description(description), beforeState(beforeState), 
      afterState(afterState), modifier(modifier), timestamp(juce::Time::getCurrentTime()) {
}

bool ProjectModificationAction::execute() {
    if (hasBeenExecuted) {
        return redo();
    }
    
    hasBeenExecuted = true;
    return modifier(afterState);
}

bool ProjectModificationAction::undo() {
    if (!hasBeenExecuted || !isReversible) {
        return false;
    }
    
    return modifier(beforeState);
}

bool ProjectModificationAction::redo() {
    if (!hasBeenExecuted || !isReversible) {
        return false;
    }
    
    return modifier(afterState);
}

size_t ProjectModificationAction::getMemoryUsage() const {
    return description.length() * sizeof(juce::String::CharPointerType) +
           beforeState.toString().length() * sizeof(juce::String::CharPointerType) +
           afterState.toString().length() * sizeof(juce::String::CharPointerType);
}

// SettingsChangeAction Implementation
SettingsChangeAction::SettingsChangeAction(const std::vector<SettingState>& changes)
    : changes(changes), timestamp(juce::Time::getCurrentTime()) {
}

juce::String SettingsChangeAction::getDescription() const {
    if (changes.empty()) return "Settings Change";
    
    if (changes.size() == 1) {
        return "Changed " + changes[0].settingId;
    }
    
    return "Changed " + juce::String(changes.size()) + " settings";
}

bool SettingsChangeAction::execute() {
    if (hasBeenExecuted) {
        return redo();
    }
    
    hasBeenExecuted = true;
    return true; // Initial change already happened
}

// SYNCHRONOUS setting apply - critical for deterministic undo/redo
// This helper applies the setting directly using thread-safe methods
static void applySettingDirect(const juce::String& id, const juce::var& value) {
    auto& settings = zenith::Settings::getInstance();
    
    // Use individual setters that handle their own thread safety
    if (id == "theme") settings.setTheme(static_cast<Settings::UITheme>(static_cast<int>(value)));
    else if (id == "renderBackend") settings.setRenderBackend(static_cast<SkiaRenderer::Backend>(static_cast<int>(value)));
    else if (id == "targetFPS") settings.setTargetFPS(static_cast<int>(value));
    else if (id == "globalScale") settings.setGlobalScale(static_cast<float>(value));
    else if (id == "glowIntensity") settings.setGlowIntensity(static_cast<float>(value));
    else if (id == "animationsEnabled") settings.setAnimationsEnabled(static_cast<bool>(value));
    else if (id == "highContrastMode") settings.setHighContrastMode(static_cast<bool>(value));
    else if (id == "bufferSize") settings.setBufferSize(static_cast<int>(value));
    else if (id == "pluginDelayCompensation") settings.setPluginDelayCompensation(static_cast<bool>(value));
    else if (id == "softwareMonitoring") settings.setSoftwareMonitoring(static_cast<bool>(value));
    else if (id == "monitoringVolume") settings.setMonitoringVolume(static_cast<float>(value));
    else if (id == "linuxAudioBackend") settings.setLinuxAudioBackend(static_cast<Settings::LinuxAudioBackend>(static_cast<int>(value)));
    else if (id == "countInBars") settings.setCountInBars(static_cast<int>(value));
    else if (id == "metronomeCountIn") settings.setMetronomeCountIn(static_cast<bool>(value));
    else if (id == "recordingBitDepth") settings.setRecordingBitDepth(static_cast<Settings::RecordingBitDepth>(static_cast<int>(value)));
    else if (id == "recordingFileType") settings.setRecordingFileType(static_cast<Settings::RecordingFileType>(static_cast<int>(value)));
    else if (id == "allowTempoChangeDuringRecord") settings.setAllowTempoChangeDuringRecord(static_cast<bool>(value));
    else if (id == "midiThrough") settings.setMIDIThrough(static_cast<bool>(value));
    else if (id == "sendMIDIClockOut") settings.setSendMIDIClockOut(static_cast<bool>(value));
    else if (id == "receiveMTCIn") settings.setReceiveMTCIn(static_cast<bool>(value));
    else if (id == "midiLatencyCompensation") settings.setMIDILatencyCompensation(static_cast<int>(value));
    else if (id == "defaultCrossfadeMs") settings.setDefaultCrossfadeMs(static_cast<int>(value));
    else if (id == "snapToGrid") settings.setSnapToGrid(static_cast<bool>(value));
    else if (id == "linkTrackAndEditSelection") settings.setLinkTrackAndEditSelection(static_cast<bool>(value));
    else if (id == "autoSaveEnabled") settings.setAutoSaveEnabled(static_cast<bool>(value));
    else if (id == "autoSaveIntervalMinutes") settings.setAutoSaveIntervalMinutes(static_cast<int>(value));
    else if (id == "maxUndoHistory") settings.setMaxUndoHistory(static_cast<int>(value));
    else if (id == "defaultProjectFolder") settings.setDefaultProjectFolder(value.toString());
    else if (id == "meterBallistics") settings.setMeterBallistics(static_cast<Settings::MeterBallistics>(static_cast<int>(value)));
    else if (id == "meterPeakHoldSeconds") settings.setMeterPeakHoldSeconds(static_cast<float>(value));
    else if (id == "showVolumeInDB") settings.setShowVolumeInDB(static_cast<bool>(value));
    else {
        juce::Logger::writeToLog("Warning: Unknown setting ID in undo: " + id);
    }
}

void SettingsChangeAction::applySetting(const juce::String& id, const juce::var& value) {
    // Settings is now thread-safe - can apply directly from any thread
    applySettingDirect(id, value);
}

bool SettingsChangeAction::undo() {
    if (!hasBeenExecuted) return false;
    
    for (const auto& change : changes) {
        applySetting(change.settingId, change.oldValue);
    }
    
    return true;
}

bool SettingsChangeAction::redo() {
    if (!hasBeenExecuted) return false;
    
    for (const auto& change : changes) {
        applySetting(change.settingId, change.newValue);
    }
    
    return true;
}

size_t SettingsChangeAction::getMemoryUsage() const {
    size_t size = sizeof(*this);
    for (const auto& change : changes) {
        size += change.settingId.length() * sizeof(juce::String::CharPointerType);
        size += change.category.length() * sizeof(juce::String::CharPointerType);
    }
    return size;
}

juce::var SettingsChangeAction::toVar() const {
    auto v = UndoableAction::toVar();
    auto* obj = v.getDynamicObject();
    
    juce::Array<juce::var> changesArray;
    for (const auto& change : changes) {
        juce::DynamicObject::Ptr c = new juce::DynamicObject();
        c->setProperty("settingId", change.settingId);
        c->setProperty("oldValue", change.oldValue);
        c->setProperty("newValue", change.newValue);
        c->setProperty("category", change.category);
        changesArray.add(juce::var(c));
    }
    obj->setProperty("changes", changesArray);
    
    return v;
}

std::unique_ptr<SettingsChangeAction> SettingsChangeAction::fromVar(const juce::var& v) {
    auto* obj = v.getDynamicObject();
    if (!obj) return nullptr;
    
    std::vector<SettingState> changes;
    auto changesArray = obj->getProperty("changes", juce::var());
    if (changesArray.isArray()) {
        for (int i = 0; i < changesArray.size(); ++i) {
            auto* c = changesArray[i].getDynamicObject();
            if (c) {
                changes.push_back({
                    c->getProperty("settingId").toString(),
                    c->getProperty("oldValue"),
                    c->getProperty("newValue"),
                    c->getProperty("category").toString()
                });
            }
        }
    }
    
    auto action = std::make_unique<SettingsChangeAction>(changes);
    action->hasBeenExecuted = true;
    return action;
}

bool SettingsChangeAction::canMergeWith(const UndoableAction& other) const {
    if (other.getType() != ActionType::SettingsChange) {
        return false;
    }
    
    const auto& otherSettings = static_cast<const SettingsChangeAction&>(other);
    
    // Can merge if same category and within time window
    if (std::abs((timestamp - otherSettings.timestamp).inMilliseconds()) > 500) {
        return false;
    }
    
    if (!changes.empty() && !otherSettings.changes.empty()) {
        return changes[0].category == otherSettings.changes[0].category;
    }
    
    return false;
}

void SettingsChangeAction::mergeWith(const UndoableAction& other) {
    const auto& otherSettings = static_cast<const SettingsChangeAction&>(other);
    
    // Update with new values
    for (auto& change1 : changes) {
        for (const auto& change2 : otherSettings.changes) {
            if (change1.settingId == change2.settingId) {
                change1.newValue = change2.newValue;
            }
        }
    }
    
    // Add new settings
    for (const auto& change2 : otherSettings.changes) {
        bool found = false;
        for (const auto& change1 : changes) {
            if (change1.settingId == change2.settingId) {
                found = true;
                break;
            }
        }
        if (!found) {
            changes.push_back(change2);
        }
    }
}

juce::String SettingsChangeAction::generateDescription() const {
    return getDescription();
}

// CustomAction Implementation
CustomAction::CustomAction(const juce::String& description,
                         ActionType type,
                         ExecuteFunc executeFunc,
                         UndoFunc undoFunc,
                         RedoFunc redoFunc)
    : description(description), actionType(type), 
      executeFunc(executeFunc), undoFunc(undoFunc), redoFunc(redoFunc),
      timestamp(juce::Time::getCurrentTime()) {
}

bool CustomAction::execute() {
    if (hasBeenExecuted) {
        return redo();
    }
    
    if (!executeFunc) return false;
    
    hasBeenExecuted = true;
    return executeFunc();
}

bool CustomAction::undo() {
    if (!hasBeenExecuted || !undoFunc) return false;
    return undoFunc();
}

bool CustomAction::redo() {
    if (!hasBeenExecuted || !redoFunc) return false;
    return redoFunc();
}

size_t CustomAction::getMemoryUsage() const {
    return description.length() * sizeof(juce::String::CharPointerType);
}

// UndoRedoSystem Implementation
UndoRedoSystem::UndoRedoSystem() {
    calculateMemoryUsage();
}

UndoRedoSystem::~UndoRedoSystem() {
    clear();
}

void UndoRedoSystem::setConfig(const SystemConfig& newConfig) {
    config = newConfig;
    enforceMemoryLimits();
}

UndoRedoSystem::SystemConfig UndoRedoSystem::getConfig() const {
    return config;
}

void UndoRedoSystem::addAction(std::unique_ptr<UndoableAction> action) {
    if (!action) return;
    
    // Clear redo stack when new action is added
    clearRedoStack();
    
    // Try to merge with last action if auto-merge is enabled
    if (config.autoMerge && !undoStack.empty()) {
        if (shouldMergeActions(*action, *undoStack.back())) {
            mergeActions(action, undoStack.back());
            calculateMemoryUsage();
            return;
        }
    }
    
    addToUndoStack(std::move(action));
    enforceMemoryLimits();
}

void UndoRedoSystem::executeAndAdd(std::unique_ptr<UndoableAction> action) {
    if (!action) return;
    
    if (action->execute()) {
        addAction(std::move(action));
    }
}

bool UndoRedoSystem::undo() {
    if (undoStack.empty()) return false;
    
    auto action = std::move(undoStack.back());
    undoStack.pop_back();
    
    bool success = action->undo();
    
    if (success) {
        addToRedoStack(std::move(action));
        notifyActionUndone(*action);
    } else {
        // Put it back if undo failed
        undoStack.push_back(std::move(action));
    }
    
    return success;
}

bool UndoRedoSystem::redo() {
    if (redoStack.empty()) return false;
    
    auto action = std::move(redoStack.back());
    redoStack.pop_back();
    
    bool success = action->redo();
    
    if (success) {
        addToUndoStack(std::move(action));
        notifyActionRedone(*action);
    } else {
        // Put it back if redo failed
        redoStack.push_back(std::move(action));
    }
    
    return success;
}

bool UndoRedoSystem::canUndo() const {
    return !undoStack.empty();
}

bool UndoRedoSystem::canRedo() const {
    return !redoStack.empty();
}

juce::String UndoRedoSystem::getUndoDescription() const {
    if (undoStack.empty()) return "";
    return undoStack.back()->getDescription();
}

juce::String UndoRedoSystem::getRedoDescription() const {
    if (redoStack.empty()) return "";
    return redoStack.back()->getDescription();
}

std::vector<juce::String> UndoRedoSystem::getUndoHistory(int maxItems) const {
    std::vector<juce::String> history;
    
    int count = juce::jmin(maxItems, static_cast<int>(undoStack.size()));
    for (int i = count - 1; i >= 0; --i) {
        history.push_back(undoStack[i]->getDescription());
    }
    
    return history;
}

std::vector<juce::String> UndoRedoSystem::getRedoHistory(int maxItems) const {
    std::vector<juce::String> history;
    
    int count = juce::jmin(maxItems, static_cast<int>(redoStack.size()));
    for (int i = 0; i < count; ++i) {
        history.push_back(redoStack[i]->getDescription());
    }
    
    return history;
}

void UndoRedoSystem::beginBatch(const juce::String& batchDescription) {
    currentBatchDescription = std::make_unique<juce::String>(batchDescription);
    batchActions.clear();
}

void UndoRedoSystem::endBatch() {
    if (!currentBatchDescription || batchActions.empty()) {
        currentBatchDescription.reset();
        return;
    }
    
    // Create a composite action
    auto compositeAction = std::make_unique<CustomAction>(
        *currentBatchDescription,
        ActionType::Custom,
        [this, actions = batchActions]() {
            for (auto& action : actions) {
                if (!action->execute()) return false;
            }
            return true;
        },
        [this, actions = batchActions]() {
            for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
                if (!(*it)->undo()) return false;
            }
            return true;
        },
        [this, actions = batchActions]() {
            for (auto& action : actions) {
                if (!action->redo()) return false;
            }
            return true;
        }
    );
    
    // Calculate total memory usage
    size_t totalMemory = 0;
    for (const auto& action : batchActions) {
        totalMemory += action->getMemoryUsage();
    }
    
    // Add composite action
    addToUndoStack(std::move(compositeAction));
    currentBatchDescription.reset();
    batchActions.clear();
    
    enforceMemoryLimits();
}

bool UndoRedoSystem::isInBatch() const {
    return currentBatchDescription != nullptr;
}

void UndoRedoSystem::clear() {
    clearUndoStack();
    clearRedoStack();
    notifyHistoryCleared();
}

void UndoRedoSystem::clearUndoHistory() {
    clearUndoStack();
}

void UndoRedoSystem::clearRedoHistory() {
    clearRedoStack();
}

size_t UndoRedoSystem::getMemoryUsage() const {
    return currentMemoryUsage.load();
}

void UndoRedoSystem::optimizeMemory() {
    // Remove old actions that are unlikely to be needed
    if (undoStack.size() > config.maxUndoSteps / 2) {
        auto removeCount = undoStack.size() / 4;
        
        for (size_t i = 0; i < removeCount; ++i) {
            undoStack.erase(undoStack.begin());
        }
        
        calculateMemoryUsage();
        notifyMemoryOptimized();
    }
}

bool UndoRedoSystem::saveHistory(const juce::File& filePath) const {
    try {
        juce::DynamicObject::Ptr historyData = new juce::DynamicObject();
        
        // Save undo stack
        juce::Array<juce::var> undoArray;
        for (const auto& action : undoStack) {
            undoArray.add(action->toVar());
        }
        historyData->setProperty("undoStack", undoArray);
        
        // Save redo stack
        juce::Array<juce::var> redoArray;
        for (const auto& action : redoStack) {
            redoArray.add(action->toVar());
        }
        historyData->setProperty("redoStack", redoArray);
        
        return filePath.replaceWithText(juce::JSON::toString(historyData));
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("UndoRedoSystem::saveHistory failed: " + juce::String(e.what()));
        return false;
    }
}

std::unique_ptr<UndoableAction> UndoRedoSystem::createActionFromVar(const juce::var& v) {
    auto* obj = v.getDynamicObject();
    if (!obj) return nullptr;
    
    auto type = static_cast<ActionType>(static_cast<int>(obj->getProperty("type", 0)));
    
    switch (type) {
        case ActionType::ParameterChange: return ParameterChangeAction::fromVar(v);
        case ActionType::SettingsChange: return SettingsChangeAction::fromVar(v);
        default: break;
    }
    
    return nullptr;
}

bool UndoRedoSystem::loadHistory(const juce::File& filePath) {
    try {
        auto content = filePath.loadFileAsString();
        auto data = juce::JSON::parse(content);
        
        if (!data.isObject()) return false;
        
        clear();
        
        // Load actions
        auto undoArray = data.getProperty("undoStack", juce::var());
        if (undoArray.isArray()) {
            for (int i = 0; i < undoArray.size(); ++i) {
                if (auto action = createActionFromVar(undoArray[i])) {
                    undoStack.push_back(std::move(action));
                }
            }
        }
        
        auto redoArray = data.getProperty("redoStack", juce::var());
        if (redoArray.isArray()) {
            for (int i = 0; i < redoArray.size(); ++i) {
                if (auto action = createActionFromVar(redoArray[i])) {
                    redoStack.push_back(std::move(action));
                }
            }
        }
        
        calculateMemoryUsage();
        return true;
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("UndoRedoSystem::loadHistory failed: " + juce::String(e.what()));
        return false;
    }
}

int UndoRedoSystem::getUndoCount() const {
    return static_cast<int>(undoStack.size());
}

int UndoRedoSystem::getRedoCount() const {
    return static_cast<int>(redoStack.size());
}

size_t UndoRedoSystem::getTotalActions() const {
    return undoStack.size() + redoStack.size();
}

juce::Time UndoRedoSystem::getOldestActionTime() const {
    if (undoStack.empty()) return juce::Time();
    
    juce::Time oldest = undoStack[0]->getTimestamp();
    for (const auto& action : undoStack) {
        if (action->getTimestamp() < oldest) {
            oldest = action->getTimestamp();
        }
    }
    
    return oldest;
}

juce::Time UndoRedoSystem::getNewestActionTime() const {
    if (undoStack.empty()) return juce::Time();
    
    juce::Time newest = undoStack[0]->getTimestamp();
    for (const auto& action : undoStack) {
        if (action->getTimestamp() > newest) {
            newest = action->getTimestamp();
        }
    }
    
    return newest;
}

void UndoRedoSystem::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void UndoRedoSystem::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

std::unique_ptr<AudioProcessingAction> UndoRedoSystem::createAudioAction(
    const juce::String& description,
    const AudioProcessingAction::AudioState& before,
    const AudioProcessingAction::AudioState& after,
    std::function<bool(const AudioProcessingAction::AudioState&)> processor) {
    return std::make_unique<AudioProcessingAction>(description, before, after, processor);
}

std::unique_ptr<ParameterChangeAction> UndoRedoSystem::createParameterAction(
    const juce::String& parameterId,
    const juce::var& oldValue,
    const juce::var& newValue,
    const juce::String& componentId) {
    
    std::vector<ParameterChangeAction::ParameterState> changes;
    changes.push_back({parameterId, oldValue, newValue, componentId});
    
    return std::make_unique<ParameterChangeAction>(changes);
}

std::unique_ptr<SettingsChangeAction> UndoRedoSystem::createSettingsAction(
    const juce::String& settingId,
    const juce::var& oldValue,
    const juce::var& newValue,
    const juce::String& category) {
    
    std::vector<SettingsChangeAction::SettingState> changes;
    changes.push_back({settingId, oldValue, newValue, category});
    
    return std::make_unique<SettingsChangeAction>(changes);
}

void UndoRedoSystem::addToUndoStack(std::unique_ptr<UndoableAction> action) {
    if (currentBatchDescription) {
        batchActions.push_back(std::move(action));
    } else {
        undoStack.push_back(std::move(action));
        notifyActionAdded(*undoStack.back());
    }
    
    calculateMemoryUsage();
}

void UndoRedoSystem::addToRedoStack(std::unique_ptr<UndoableAction> action) {
    redoStack.push_back(std::move(action));
    calculateMemoryUsage();
}

void UndoRedoSystem::optimizeMemoryUsage() {
    if (currentMemoryUsage.load() > config.maxMemoryUsage) {
        optimizeMemory();
    }
}

bool UndoRedoSystem::shouldMergeActions(const UndoableAction& newer, const UndoableAction& older) const {
    return newer.canMergeWith(older);
}

void UndoRedoSystem::mergeActions(std::unique_ptr<UndoableAction>& newer, std::unique_ptr<UndoableAction>& older) {
    older->mergeWith(*newer);
}

void UndoRedoSystem::enforceMemoryLimits() {
    // Enforce action count limit
    while (undoStack.size() > config.maxUndoSteps) {
        undoStack.erase(undoStack.begin());
    }
    
    while (redoStack.size() > config.maxUndoSteps) {
        redoStack.erase(redoStack.begin());
    }
    
    // Enforce memory limit
    optimizeMemoryUsage();
}

void UndoRedoSystem::calculateMemoryUsage() const {
    size_t total = 0;
    
    for (const auto& action : undoStack) {
        total += action->getMemoryUsage();
    }
    
    for (const auto& action : redoStack) {
        total += action->getMemoryUsage();
    }
    
    currentMemoryUsage.store(total);
}

void UndoRedoSystem::clearUndoStack() {
    undoStack.clear();
    calculateMemoryUsage();
}

void UndoRedoSystem::clearRedoStack() {
    redoStack.clear();
    calculateMemoryUsage();
}

void UndoRedoSystem::notifyActionAdded(const UndoableAction& action) {
    for (auto* listener : listeners) {
        listener->actionAdded(action);
    }
}

void UndoRedoSystem::notifyActionUndone(const UndoableAction& action) {
    for (auto* listener : listeners) {
        listener->actionUndone(action);
    }
}

void UndoRedoSystem::notifyActionRedone(const UndoableAction& action) {
    for (auto* listener : listeners) {
        listener->actionRedone(action);
    }
}

void UndoRedoSystem::notifyHistoryCleared() {
    for (auto* listener : listeners) {
        listener->historyCleared();
    }
}

void UndoRedoSystem::notifyMemoryOptimized() {
    for (auto* listener : listeners) {
        listener->memoryOptimized();
    }
}

// UndoRedoManager Implementation
UndoRedoManager::UndoRedoManager()
    : undoRedoSystem(std::make_unique<UndoRedoSystem>()) {
}

UndoRedoManager::~UndoRedoManager() = default;

UndoRedoSystem& UndoRedoManager::getSystem() {
    return *undoRedoSystem;
}

void UndoRedoManager::setUndoButton(juce::Button* button) {
    undoButton = button;
    if (button) {
        button->addListener(this);
        updateUndoButton();
    }
}

void UndoRedoManager::setRedoButton(juce::Button* button) {
    redoButton = button;
    if (button) {
        button->addListener(this);
        updateRedoButton();
    }
}

void UndoRedoManager::setUndoMenu(juce::PopupMenu* menu) {
    undoMenu = menu;
    updateUndoMenu();
}

void UndoRedoManager::setRedoMenu(juce::PopupMenu* menu) {
    redoMenu = menu;
    updateRedoMenu();
}

void UndoRedoManager::enableAutoUpdate(bool enabled) {
    autoUpdateEnabled = enabled;
}

bool UndoRedoManager::isAutoUpdateEnabled() const {
    return autoUpdateEnabled;
}

void UndoRedoManager::setUndoKey(const juce::KeyPress& key) {
    undoKey = key;
}

void UndoRedoManager::setRedoKey(const juce::KeyPress& key) {
    redoKey = key;
}

bool UndoRedoManager::handleKeyPress(const juce::KeyPress& key) {
    if (key == undoKey) {
        undoRedoSystem->undo();
        return true;
    } else if (key == redoKey) {
        undoRedoSystem->redo();
        return true;
    }
    
    return false;
}

void UndoRedoManager::setStatusLabel(juce::Label* label) {
    statusLabel = label;
    updateStatusLabel();
}

void UndoRedoManager::recordAudioProcessing(const juce::String& description,
                                         const juce::AudioBuffer<float>& beforeAudio,
                                         const juce::AudioBuffer<float>& afterAudio,
                                         double sampleRate,
                                         std::function<bool(const juce::AudioBuffer<float>&)> processor) {
    
    AudioProcessingAction::AudioState beforeState;
    beforeState.audioBuffer = beforeAudio;
    beforeState.sampleRate = sampleRate;
    
    AudioProcessingAction::AudioState afterState;
    afterState.audioBuffer = afterAudio;
    afterState.sampleRate = sampleRate;
    
    auto action = UndoRedoSystem::createAudioAction(description, beforeState, afterState,
        [processor](const AudioProcessingAction::AudioState& state) {
            return processor(state.audioBuffer);
        });
    
    undoRedoSystem->executeAndAdd(std::move(action));
}

void UndoRedoManager::recordParameterChange(const juce::String& parameterId,
                                           const juce::var& oldValue,
                                           const juce::var& newValue,
                                           const juce::String& componentId) {
    auto action = UndoRedoSystem::createParameterAction(parameterId, oldValue, newValue, componentId);
    undoRedoSystem->executeAndAdd(std::move(action));
}

void UndoRedoManager::recordSettingsChange(const juce::String& settingId,
                                         const juce::var& oldValue,
                                         const juce::var& newValue,
                                         const juce::String& category) {
    auto action = UndoRedoSystem::createSettingsAction(settingId, oldValue, newValue, category);
    undoRedoSystem->executeAndAdd(std::move(action));
}

bool UndoRedoManager::keyPressed(const juce::KeyPress& key) {
    return handleKeyPress(key);
}

void UndoRedoManager::updateUndoButton() {
    if (undoButton && autoUpdateEnabled) {
        undoButton->setEnabled(undoRedoSystem->canUndo());
        undoButton->setTooltip(undoRedoSystem->getUndoDescription());
    }
}

void UndoRedoManager::updateRedoButton() {
    if (redoButton && autoUpdateEnabled) {
        redoButton->setEnabled(undoRedoSystem->canRedo());
        redoButton->setTooltip(undoRedoSystem->getRedoDescription());
    }
}

void UndoRedoManager::updateUndoMenu() {
    if (undoMenu && autoUpdateEnabled) {
        undoMenu->clear();
        
        auto history = undoRedoSystem->getUndoHistory(20);
        for (int i = 0; i < history.size(); ++i) {
            undoMenu->addItem(i + 1, history[i]);
        }
    }
}

void UndoRedoManager::updateRedoMenu() {
    if (redoMenu && autoUpdateEnabled) {
        redoMenu->clear();
        
        auto history = undoRedoSystem->getRedoHistory(20);
        for (int i = 0; i < history.size(); ++i) {
            redoMenu->addItem(i + 1, history[i]);
        }
    }
}

void UndoRedoManager::updateStatusLabel() {
    if (statusLabel && autoUpdateEnabled) {
        juce::String text = "Undo: " + undoRedoSystem->getUndoDescription();
        if (undoRedoSystem->canRedo()) {
            text += " | Redo: " + undoRedoSystem->getRedoDescription();
        }
        statusLabel->setText(text, juce::dontSendNotification);
    }
}

void UndoRedoManager::onUndoButtonClicked() {
    undoRedoSystem->undo();
    updateAll();
}

void UndoRedoManager::onRedoButtonClicked() {
    undoRedoSystem->redo();
    updateAll();
}

void UndoRedoManager::onUndoMenuItemSelected(int menuItemId) {
    // Undo multiple steps
    for (int i = 0; i < menuItemId; ++i) {
        if (!undoRedoSystem->undo()) break;
    }
    updateAll();
}

void UndoRedoManager::onRedoMenuItemSelected(int menuItemId) {
    // Redo multiple steps
    for (int i = 0; i < menuItemId; ++i) {
        if (!undoRedoSystem->redo()) break;
    }
    updateAll();
}

void UndoRedoManager::updateAll() {
    updateUndoButton();
    updateRedoButton();
    updateUndoMenu();
    updateRedoMenu();
    updateStatusLabel();
}

// Global instance management
std::unique_ptr<UndoRedoSystem> GlobalUndoRedo::instance;
std::unique_ptr<UndoRedoManager> GlobalUndoRedo::manager;
std::mutex GlobalUndoRedo::mutex;

UndoRedoSystem& GlobalUndoRedo::getInstance() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!instance) {
        instance = std::make_unique<UndoRedoSystem>();
    }
    return *instance;
}

UndoRedoManager& GlobalUndoRedo::getManager() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!manager) {
        manager = std::make_unique<UndoRedoManager>();
    }
    return *manager;
}

} // namespace ui
} // namespace zenith
