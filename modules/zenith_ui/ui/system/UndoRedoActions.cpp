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

#include "zenith_core/engine/Settings.h"

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
    if (hasBeenExecuted) return redo();
    hasBeenExecuted = true;
    return processor(afterState);
}

bool AudioProcessingAction::undo() {
    if (!hasBeenExecuted || !isReversible) return false;
    return processor(beforeState);
}

bool AudioProcessingAction::redo() {
    if (!hasBeenExecuted || !isReversible) return false;
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
    if (changes.size() == 1) return "Changed " + changes[0].parameterId;
    return "Changed " + juce::String(changes.size()) + " parameters";
}

bool ParameterChangeAction::execute() {
    if (hasBeenExecuted) return redo();
    hasBeenExecuted = true;
    return true; 
}

bool ParameterChangeAction::undo() {
    if (!hasBeenExecuted) return false;
    auto* engine = Engine::getInstance();
    if (!engine) return false;
    auto* projectState = engine->getProjectState();
    if (!projectState) return false;
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
    if (other.getType() != ActionType::ParameterChange) return false;
    const auto& otherParam = static_cast<const ParameterChangeAction&>(other);
    if (std::abs((timestamp - otherParam.timestamp).inMilliseconds()) > 1000) return false;
    for (const auto& change1 : changes) {
        for (const auto& change2 : otherParam.changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) return true;
        }
    }
    return false;
}

void ParameterChangeAction::mergeWith(const UndoableAction& other) {
    const auto& otherParam = static_cast<const ParameterChangeAction&>(other);
    for (auto& change1 : changes) {
        for (const auto& change2 : otherParam.changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) {
                change1.newValue = change2.newValue;
            }
        }
    }
    for (const auto& change2 : otherParam.changes) {
        bool found = false;
        for (const auto& change1 : changes) {
            if (change1.parameterId == change2.parameterId && 
                change1.componentId == change2.componentId) {
                found = true;
                break;
            }
        }
        if (!found) changes.push_back(change2);
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
    if (hasBeenExecuted) return redo();
    hasBeenExecuted = true;
    return modifier(afterState);
}

bool ProjectModificationAction::undo() {
    if (!hasBeenExecuted || !isReversible) return false;
    return modifier(beforeState);
}

bool ProjectModificationAction::redo() {
    if (!hasBeenExecuted || !isReversible) return false;
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
    if (changes.size() == 1) return "Changed " + changes[0].settingId;
    return "Changed " + juce::String(changes.size()) + " settings";
}

bool SettingsChangeAction::execute() {
    if (hasBeenExecuted) return redo();
    hasBeenExecuted = true;
    return true; 
}

static void applySettingDirect(const juce::String& id, const juce::var& value) {
    auto& settings = zenith::Settings::getInstance();
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
    else juce::Logger::writeToLog("Warning: Unknown setting ID in undo: " + id);
}

void SettingsChangeAction::applySetting(const juce::String& id, const juce::var& value) {
    applySettingDirect(id, value);
}

bool SettingsChangeAction::undo() {
    if (!hasBeenExecuted) return false;
    for (const auto& change : changes) applySetting(change.settingId, change.oldValue);
    return true;
}

bool SettingsChangeAction::redo() {
    if (!hasBeenExecuted) return false;
    for (const auto& change : changes) applySetting(change.settingId, change.newValue);
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
    if (other.getType() != ActionType::SettingsChange) return false;
    const auto& otherSettings = static_cast<const SettingsChangeAction&>(other);
    if (std::abs((timestamp - otherSettings.timestamp).inMilliseconds()) > 500) return false;
    if (!changes.empty() && !otherSettings.changes.empty()) {
        return changes[0].category == otherSettings.changes[0].category;
    }
    return false;
}

void SettingsChangeAction::mergeWith(const UndoableAction& other) {
    const auto& otherSettings = static_cast<const SettingsChangeAction&>(other);
    for (auto& change1 : changes) {
        for (const auto& change2 : otherSettings.changes) {
            if (change1.settingId == change2.settingId) change1.newValue = change2.newValue;
        }
    }
    for (const auto& change2 : otherSettings.changes) {
        bool found = false;
        for (const auto& change1 : changes) {
            if (change1.settingId == change2.settingId) {
                found = true;
                break;
            }
        }
        if (!found) changes.push_back(change2);
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
    if (hasBeenExecuted) return redo();
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

} // namespace ui
} // namespace zenith
