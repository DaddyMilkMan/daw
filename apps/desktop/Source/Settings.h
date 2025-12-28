/*
  ==============================================================================

    Settings.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Global application settings.
    Acts as a persistent store and data model for preferences.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "rendering/SkiaRenderer.h"
#include <mutex>
#include <functional>

namespace zenith {

class Settings : public juce::ChangeBroadcaster {
public:
    // ===== ENUMS =====
    enum class LinuxAudioBackend { Auto, JACK, PipeWire, ALSA };
    enum class RecordingBitDepth { Bit16, Bit24, Bit32Float };
    enum class RecordingFileType { WAV, AIFF, FLAC };
    enum class MeterBallistics { VU, PPM, Peak };
    enum class UITheme { Neon, Dark, Light };

    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    // Thread-safe accessors - CRITICAL: sendChangeMessage() outside lock to prevent deadlock!
    template<typename Func>
    auto withLock(Func&& func) -> decltype(func()) {
        std::lock_guard<std::mutex> lock(settingsMutex);
        return func();
    }
    
    // Thread-safe setter that broadcasts outside lock
    template<typename SetValueFunc>
    void setWithBroadcast(SetValueFunc&& setValueFunc) {
        bool shouldBroadcast = false;
        {
            std::lock_guard<std::mutex> lock(settingsMutex);
            shouldBroadcast = setValueFunc();
        }
        if (shouldBroadcast) {
            sendChangeMessage();
        }
    }

    //==============================================================================
    // Persistence
    //==============================================================================
    void load() {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = ".settings";
        options.folderName = "ZenithAudio";
        options.osxLibrarySubFolder = "Application Support";
        
        juce::ApplicationProperties props;
        props.setStorageParameters(options);
        
        if (auto* userSettings = props.getUserSettings()) {
            // Display
            renderBackend_ = (SkiaRenderer::Backend)userSettings->getIntValue("renderBackend", (int)SkiaRenderer::Backend::Auto);
            targetFPS_ = userSettings->getIntValue("targetFPS", 60);
            globalScale_ = (float)userSettings->getDoubleValue("globalScale", 1.0);
            glowIntensity_ = (float)userSettings->getDoubleValue("glowIntensity", 1.0);
            theme_ = (UITheme)userSettings->getIntValue("theme", (int)UITheme::Neon);
            animationsEnabled_ = userSettings->getBoolValue("animationsEnabled", true);
            highContrastMode_ = userSettings->getBoolValue("highContrastMode", false);
            
            // Audio
            linuxAudioBackend_ = (LinuxAudioBackend)userSettings->getIntValue("linuxAudioBackend", (int)LinuxAudioBackend::Auto);
            bufferSize_ = userSettings->getIntValue("bufferSize", 512);
            pluginDelayCompensation_ = userSettings->getBoolValue("pluginDelayCompensation", true);
            softwareMonitoring_ = userSettings->getBoolValue("softwareMonitoring", true);
            monitoringVolume_ = (float)userSettings->getDoubleValue("monitoringVolume", 1.0);
            
            // Recording
            countInBars_ = userSettings->getIntValue("countInBars", 1);
            metronomeCountIn_ = userSettings->getBoolValue("metronomeCountIn", true);
            recordingBitDepth_ = (RecordingBitDepth)userSettings->getIntValue("recordingBitDepth", (int)RecordingBitDepth::Bit24);
            recordingFileType_ = (RecordingFileType)userSettings->getIntValue("recordingFileType", (int)RecordingFileType::WAV);
            allowTempoChangeDuringRecord_ = userSettings->getBoolValue("allowTempoChangeDuringRecord", false);
            
            // MIDI
            midiThrough_ = userSettings->getBoolValue("midiThrough", true);
            sendMIDIClockOut_ = userSettings->getBoolValue("sendMIDIClockOut", false);
            receiveMTCIn_ = userSettings->getBoolValue("receiveMTCIn", false);
            midiLatencyCompensation_ = userSettings->getIntValue("midiLatencyCompensation", 0);
            
            // Editing
            defaultCrossfadeMs_ = userSettings->getIntValue("defaultCrossfadeMs", 10);
            snapToGrid_ = userSettings->getBoolValue("snapToGrid", true);
            linkTrackAndEditSelection_ = userSettings->getBoolValue("linkTrackAndEditSelection", true);
            
            // Project
            autoSaveEnabled_ = userSettings->getBoolValue("autoSaveEnabled", true);
            autoSaveIntervalMinutes_ = userSettings->getIntValue("autoSaveIntervalMinutes", 5);
            maxUndoHistory_ = userSettings->getIntValue("maxUndoHistory", 100);
            defaultProjectFolder_ = userSettings->getValue("defaultProjectFolder", "");
            
            // Metering
            meterBallistics_ = (MeterBallistics)userSettings->getIntValue("meterBallistics", (int)MeterBallistics::Peak);
            meterPeakHoldSeconds_ = (float)userSettings->getDoubleValue("meterPeakHoldSeconds", 2.0);
            showVolumeInDB_ = userSettings->getBoolValue("showVolumeInDB", true);
        }
    }

    void save() {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = ".settings";
        options.folderName = "ZenithAudio";
        options.osxLibrarySubFolder = "Application Support";
        
        juce::ApplicationProperties props;
        props.setStorageParameters(options);
        
        if (auto* userSettings = props.getUserSettings()) {
            // Display
            userSettings->setValue("renderBackend", (int)renderBackend_);
            userSettings->setValue("targetFPS", targetFPS_);
            userSettings->setValue("globalScale", globalScale_);
            userSettings->setValue("glowIntensity", glowIntensity_);
            userSettings->setValue("theme", (int)theme_);
            userSettings->setValue("animationsEnabled", animationsEnabled_);
            userSettings->setValue("highContrastMode", highContrastMode_);
            
            // Audio
            userSettings->setValue("linuxAudioBackend", (int)linuxAudioBackend_);
            userSettings->setValue("bufferSize", bufferSize_);
            userSettings->setValue("pluginDelayCompensation", pluginDelayCompensation_);
            userSettings->setValue("softwareMonitoring", softwareMonitoring_);
            userSettings->setValue("monitoringVolume", monitoringVolume_);
            
            // Recording
            userSettings->setValue("countInBars", countInBars_);
            userSettings->setValue("metronomeCountIn", metronomeCountIn_);
            userSettings->setValue("recordingBitDepth", (int)recordingBitDepth_);
            userSettings->setValue("recordingFileType", (int)recordingFileType_);
            userSettings->setValue("allowTempoChangeDuringRecord", allowTempoChangeDuringRecord_);
            
            // MIDI
            userSettings->setValue("midiThrough", midiThrough_);
            userSettings->setValue("sendMIDIClockOut", sendMIDIClockOut_);
            userSettings->setValue("receiveMTCIn", receiveMTCIn_);
            userSettings->setValue("midiLatencyCompensation", midiLatencyCompensation_);
            
            // Editing
            userSettings->setValue("defaultCrossfadeMs", defaultCrossfadeMs_);
            userSettings->setValue("snapToGrid", snapToGrid_);
            userSettings->setValue("linkTrackAndEditSelection", linkTrackAndEditSelection_);
            
            // Project
            userSettings->setValue("autoSaveEnabled", autoSaveEnabled_);
            userSettings->setValue("autoSaveIntervalMinutes", autoSaveIntervalMinutes_);
            userSettings->setValue("maxUndoHistory", maxUndoHistory_);
            userSettings->setValue("defaultProjectFolder", defaultProjectFolder_);
            
            // Metering
            userSettings->setValue("meterBallistics", (int)meterBallistics_);
            userSettings->setValue("meterPeakHoldSeconds", meterPeakHoldSeconds_);
            userSettings->setValue("showVolumeInDB", showVolumeInDB_);
            
            userSettings->saveIfNeeded();
        }
    }

    //==============================================================================
    // Display Settings
    //==============================================================================
    void setRenderBackend(SkiaRenderer::Backend backend) {
        setWithBroadcast([&]() { if (renderBackend_ != backend) { renderBackend_ = backend; save(); return true; } return false; });
    }
    SkiaRenderer::Backend getRenderBackend() const { std::lock_guard<std::mutex> lock(settingsMutex); return renderBackend_; }

    void setTargetFPS(int fps) {
        setWithBroadcast([&]() { if (targetFPS_ != fps) { targetFPS_ = fps; save(); return true; } return false; });
    }
    int getTargetFPS() const { std::lock_guard<std::mutex> lock(settingsMutex); return targetFPS_; }

    void setGlobalScale(float scale) {
        setWithBroadcast([&]() { if (globalScale_ != scale) { globalScale_ = scale; save(); return true; } return false; });
    }
    float getGlobalScale() const { std::lock_guard<std::mutex> lock(settingsMutex); return globalScale_; }

    void setGlowIntensity(float intensity) {
        setWithBroadcast([&]() { if (glowIntensity_ != intensity) { glowIntensity_ = intensity; save(); return true; } return false; });
    }
    float getGlowIntensity() const { std::lock_guard<std::mutex> lock(settingsMutex); return glowIntensity_; }

    void setTheme(UITheme theme) {
        setWithBroadcast([&]() {
            if (theme_ != theme) {
                theme_ = theme;
                save();
                return true;
            }
            return false;
        });
    }
    UITheme getTheme() const { return theme_; }

    void setAnimationsEnabled(bool enabled) {
        setWithBroadcast([&]() {
            if (animationsEnabled_ != enabled) {
                animationsEnabled_ = enabled;
                save();
                return true;
            }
            return false;
        });
    }
    bool getAnimationsEnabled() const { return animationsEnabled_; }

    void setHighContrastMode(bool enabled) { if (highContrastMode_ != enabled) { highContrastMode_ = enabled; save(); sendChangeMessage(); } }
    bool getHighContrastMode() const { return highContrastMode_; }

    //==============================================================================
    // Audio Settings
    //==============================================================================
    void setLinuxAudioBackend(LinuxAudioBackend backend) {
        setWithBroadcast([&]() { if (linuxAudioBackend_ != backend) { linuxAudioBackend_ = backend; save(); return true; } return false; });
    }
    LinuxAudioBackend getLinuxAudioBackend() const { return linuxAudioBackend_; }

    void setBufferSize(int size) {
        setWithBroadcast([&]() { if (bufferSize_ != size) { bufferSize_ = size; save(); return true; } return false; });
    }
    int getBufferSize() const { return bufferSize_; }

    void setPluginDelayCompensation(bool enabled) {
        setWithBroadcast([&]() { if (pluginDelayCompensation_ != enabled) { pluginDelayCompensation_ = enabled; save(); return true; } return false; });
    }
    bool getPluginDelayCompensation() const { return pluginDelayCompensation_; }

    void setSoftwareMonitoring(bool enabled) {
        setWithBroadcast([&]() { if (softwareMonitoring_ != enabled) { softwareMonitoring_ = enabled; save(); return true; } return false; });
    }
    bool getSoftwareMonitoring() const { return softwareMonitoring_; }

    void setMonitoringVolume(float vol) {
        setWithBroadcast([&]() { if (monitoringVolume_ != vol) { monitoringVolume_ = vol; save(); return true; } return false; });
    }
    float getMonitoringVolume() const { return monitoringVolume_; }

    //==============================================================================
    // Recording Settings
    //==============================================================================
    void setCountInBars(int bars) {
        setWithBroadcast([&]() { if (countInBars_ != bars) { countInBars_ = bars; save(); return true; } return false; });
    }
    int getCountInBars() const { return countInBars_; }

    void setMetronomeCountIn(bool enabled) {
        setWithBroadcast([&]() { if (metronomeCountIn_ != enabled) { metronomeCountIn_ = enabled; save(); return true; } return false; });
    }
    bool getMetronomeCountIn() const { return metronomeCountIn_; }

    void setRecordingBitDepth(RecordingBitDepth depth) {
        setWithBroadcast([&]() { if (recordingBitDepth_ != depth) { recordingBitDepth_ = depth; save(); return true; } return false; });
    }
    RecordingBitDepth getRecordingBitDepth() const { return recordingBitDepth_; }

    void setRecordingFileType(RecordingFileType type) {
        setWithBroadcast([&]() { if (recordingFileType_ != type) { recordingFileType_ = type; save(); return true; } return false; });
    }
    RecordingFileType getRecordingFileType() const { return recordingFileType_; }

    void setAllowTempoChangeDuringRecord(bool allow) {
        setWithBroadcast([&]() { if (allowTempoChangeDuringRecord_ != allow) { allowTempoChangeDuringRecord_ = allow; save(); return true; } return false; });
    }
    bool getAllowTempoChangeDuringRecord() const { return allowTempoChangeDuringRecord_; }

    //==============================================================================
    // MIDI Settings
    //==============================================================================
    void setMIDIThrough(bool enabled) {
        setWithBroadcast([&]() { if (midiThrough_ != enabled) { midiThrough_ = enabled; save(); return true; } return false; });
    }
    bool getMIDIThrough() const { return midiThrough_; }

    void setSendMIDIClockOut(bool enabled) {
        setWithBroadcast([&]() { if (sendMIDIClockOut_ != enabled) { sendMIDIClockOut_ = enabled; save(); return true; } return false; });
    }
    bool getSendMIDIClockOut() const { return sendMIDIClockOut_; }

    void setReceiveMTCIn(bool enabled) {
        setWithBroadcast([&]() { if (receiveMTCIn_ != enabled) { receiveMTCIn_ = enabled; save(); return true; } return false; });
    }
    bool getReceiveMTCIn() const { return receiveMTCIn_; }

    void setMIDILatencyCompensation(int ms) {
        setWithBroadcast([&]() { if (midiLatencyCompensation_ != ms) { midiLatencyCompensation_ = ms; save(); return true; } return false; });
    }
    int getMIDILatencyCompensation() const { return midiLatencyCompensation_; }

    //==============================================================================
    // Editing Settings
    //==============================================================================
    void setDefaultCrossfadeMs(int ms) {
        setWithBroadcast([&]() { if (defaultCrossfadeMs_ != ms) { defaultCrossfadeMs_ = ms; save(); return true; } return false; });
    }
    int getDefaultCrossfadeMs() const { return defaultCrossfadeMs_; }

    void setSnapToGrid(bool enabled) {
        setWithBroadcast([&]() { if (snapToGrid_ != enabled) { snapToGrid_ = enabled; save(); return true; } return false; });
    }
    bool getSnapToGrid() const { return snapToGrid_; }

    void setLinkTrackAndEditSelection(bool enabled) {
        setWithBroadcast([&]() { if (linkTrackAndEditSelection_ != enabled) { linkTrackAndEditSelection_ = enabled; save(); return true; } return false; });
    }
    bool getLinkTrackAndEditSelection() const { return linkTrackAndEditSelection_; }

    //==============================================================================
    // Project Settings
    //==============================================================================
    void setAutoSaveEnabled(bool enabled) {
        setWithBroadcast([&]() { if (autoSaveEnabled_ != enabled) { autoSaveEnabled_ = enabled; save(); return true; } return false; });
    }
    bool getAutoSaveEnabled() const { return autoSaveEnabled_; }

    void setAutoSaveIntervalMinutes(int mins) {
        setWithBroadcast([&]() { if (autoSaveIntervalMinutes_ != mins) { autoSaveIntervalMinutes_ = mins; save(); return true; } return false; });
    }
    int getAutoSaveIntervalMinutes() const { return autoSaveIntervalMinutes_; }

    void setMaxUndoHistory(int max) {
        setWithBroadcast([&]() { if (maxUndoHistory_ != max) { maxUndoHistory_ = max; save(); return true; } return false; });
    }
    int getMaxUndoHistory() const { return maxUndoHistory_; }

    void setDefaultProjectFolder(const juce::String& folder) {
        setWithBroadcast([&]() { if (defaultProjectFolder_ != folder) { defaultProjectFolder_ = folder; save(); return true; } return false; });
    }
    juce::String getDefaultProjectFolder() const { return defaultProjectFolder_; }

    //==============================================================================
    // Metering Settings
    //==============================================================================
    void setMeterBallistics(MeterBallistics ballistics) {
        setWithBroadcast([&]() { if (meterBallistics_ != ballistics) { meterBallistics_ = ballistics; save(); return true; } return false; });
    }
    MeterBallistics getMeterBallistics() const { return meterBallistics_; }

    void setMeterPeakHoldSeconds(float secs) {
        setWithBroadcast([&]() { if (meterPeakHoldSeconds_ != secs) { meterPeakHoldSeconds_ = secs; save(); return true; } return false; });
    }
    float getMeterPeakHoldSeconds() const { return meterPeakHoldSeconds_; }

    void setShowVolumeInDB(bool enabled) {
        setWithBroadcast([&]() { if (showVolumeInDB_ != enabled) { showVolumeInDB_ = enabled; save(); return true; } return false; });
    }
    bool getShowVolumeInDB() const { return showVolumeInDB_; }

    //==============================================================================
    // Plugin Settings
    //==============================================================================
    // Managed by PluginHost

private:
    // Thread safety
    mutable std::mutex settingsMutex;
    
    // Data members
    Settings() = default;
    void sendChangeMessage() { juce::ChangeBroadcaster::sendChangeMessage(); }

    // Display
    SkiaRenderer::Backend renderBackend_ = SkiaRenderer::Backend::Auto;
    int targetFPS_ = 60;
    float globalScale_ = 1.0f;
    float glowIntensity_ = 1.0f;
    UITheme theme_ = UITheme::Neon;
    bool animationsEnabled_ = true;
    bool highContrastMode_ = false;

    // Audio
    LinuxAudioBackend linuxAudioBackend_ = LinuxAudioBackend::Auto;
    int bufferSize_ = 512;
    bool pluginDelayCompensation_ = true;
    bool softwareMonitoring_ = true;
    float monitoringVolume_ = 1.0f;

    // Recording
    int countInBars_ = 1;
    bool metronomeCountIn_ = true;
    RecordingBitDepth recordingBitDepth_ = RecordingBitDepth::Bit24;
    RecordingFileType recordingFileType_ = RecordingFileType::WAV;
    bool allowTempoChangeDuringRecord_ = false;

    // MIDI
    bool midiThrough_ = true;
    bool sendMIDIClockOut_ = false;
    bool receiveMTCIn_ = false;
    int midiLatencyCompensation_ = 0;

    // Editing
    int defaultCrossfadeMs_ = 10;
    bool snapToGrid_ = true;
    bool linkTrackAndEditSelection_ = true;

    // Project
    bool autoSaveEnabled_ = true;
    int autoSaveIntervalMinutes_ = 5;
    int maxUndoHistory_ = 100;
    juce::String defaultProjectFolder_;

    // Metering
    MeterBallistics meterBallistics_ = MeterBallistics::Peak;
    float meterPeakHoldSeconds_ = 2.0f;
    bool showVolumeInDB_ = true;
};

} // namespace zenith