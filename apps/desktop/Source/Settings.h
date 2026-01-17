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

            // Zenith Hub
            customGreeting_ = userSettings->getValue("customGreeting", "");

            // Features
            stayAwakeDuringProject_ = userSettings->getBoolValue("stayAwakeDuringProject", true);
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
            userSettings->setValue("meterPeakHoldSeconds", meterPeakHoldSeconds_);
            userSettings->setValue("showVolumeInDB", showVolumeInDB_);

            // Zenith Hub
            userSettings->setValue("customGreeting", customGreeting_);
            
            // Features
            userSettings->setValue("stayAwakeDuringProject", stayAwakeDuringProject_);

            userSettings->saveIfNeeded();
        }
    }

    //==============================================================================
    // Display Settings
    //==============================================================================
    void setRenderBackend(SkiaRenderer::Backend backend) { if (renderBackend_ != backend) { renderBackend_ = backend; save(); sendChangeMessage(); } }
    SkiaRenderer::Backend getRenderBackend() const { return renderBackend_; }

    void setTargetFPS(int fps) { if (targetFPS_ != fps) { targetFPS_ = fps; save(); sendChangeMessage(); } }
    int getTargetFPS() const { return targetFPS_; }

    void setGlobalScale(float scale) { if (globalScale_ != scale) { globalScale_ = scale; save(); sendChangeMessage(); } }
    float getGlobalScale() const { return globalScale_; }

    void setGlowIntensity(float intensity) { if (glowIntensity_ != intensity) { glowIntensity_ = intensity; save(); sendChangeMessage(); } }
    float getGlowIntensity() const { return glowIntensity_; }

    void setTheme(UITheme theme) { if (theme_ != theme) { theme_ = theme; save(); sendChangeMessage(); } }
    UITheme getTheme() const { return theme_; }

    void setAnimationsEnabled(bool enabled) { if (animationsEnabled_ != enabled) { animationsEnabled_ = enabled; save(); sendChangeMessage(); } }
    bool getAnimationsEnabled() const { return animationsEnabled_; }

    void setHighContrastMode(bool enabled) { if (highContrastMode_ != enabled) { highContrastMode_ = enabled; save(); sendChangeMessage(); } }
    bool getHighContrastMode() const { return highContrastMode_; }

    //==============================================================================
    // Audio Settings
    //==============================================================================
    void setLinuxAudioBackend(LinuxAudioBackend backend) { if (linuxAudioBackend_ != backend) { linuxAudioBackend_ = backend; save(); sendChangeMessage(); } }
    LinuxAudioBackend getLinuxAudioBackend() const { return linuxAudioBackend_; }

    void setBufferSize(int size) { if (bufferSize_ != size) { bufferSize_ = size; save(); sendChangeMessage(); } }
    int getBufferSize() const { return bufferSize_; }

    void setPluginDelayCompensation(bool enabled) { if (pluginDelayCompensation_ != enabled) { pluginDelayCompensation_ = enabled; save(); sendChangeMessage(); } }
    bool getPluginDelayCompensation() const { return pluginDelayCompensation_; }

    void setSoftwareMonitoring(bool enabled) { if (softwareMonitoring_ != enabled) { softwareMonitoring_ = enabled; save(); sendChangeMessage(); } }
    bool getSoftwareMonitoring() const { return softwareMonitoring_; }

    void setMonitoringVolume(float vol) { if (monitoringVolume_ != vol) { monitoringVolume_ = vol; save(); sendChangeMessage(); } }
    float getMonitoringVolume() const { return monitoringVolume_; }

    //==============================================================================
    // Recording Settings
    //==============================================================================
    void setCountInBars(int bars) { if (countInBars_ != bars) { countInBars_ = bars; save(); sendChangeMessage(); } }
    int getCountInBars() const { return countInBars_; }

    void setMetronomeCountIn(bool enabled) { if (metronomeCountIn_ != enabled) { metronomeCountIn_ = enabled; save(); sendChangeMessage(); } }
    bool getMetronomeCountIn() const { return metronomeCountIn_; }

    void setRecordingBitDepth(RecordingBitDepth depth) { if (recordingBitDepth_ != depth) { recordingBitDepth_ = depth; save(); sendChangeMessage(); } }
    RecordingBitDepth getRecordingBitDepth() const { return recordingBitDepth_; }

    void setRecordingFileType(RecordingFileType type) { if (recordingFileType_ != type) { recordingFileType_ = type; save(); sendChangeMessage(); } }
    RecordingFileType getRecordingFileType() const { return recordingFileType_; }

    void setAllowTempoChangeDuringRecord(bool allow) { if (allowTempoChangeDuringRecord_ != allow) { allowTempoChangeDuringRecord_ = allow; save(); sendChangeMessage(); } }
    bool getAllowTempoChangeDuringRecord() const { return allowTempoChangeDuringRecord_; }

    //==============================================================================
    // MIDI Settings
    //==============================================================================
    void setMIDIThrough(bool enabled) { if (midiThrough_ != enabled) { midiThrough_ = enabled; save(); sendChangeMessage(); } }
    bool getMIDIThrough() const { return midiThrough_; }

    void setSendMIDIClockOut(bool enabled) { if (sendMIDIClockOut_ != enabled) { sendMIDIClockOut_ = enabled; save(); sendChangeMessage(); } }
    bool getSendMIDIClockOut() const { return sendMIDIClockOut_; }

    void setReceiveMTCIn(bool enabled) { if (receiveMTCIn_ != enabled) { receiveMTCIn_ = enabled; save(); sendChangeMessage(); } }
    bool getReceiveMTCIn() const { return receiveMTCIn_; }

    void setMIDILatencyCompensation(int ms) { if (midiLatencyCompensation_ != ms) { midiLatencyCompensation_ = ms; save(); sendChangeMessage(); } }
    int getMIDILatencyCompensation() const { return midiLatencyCompensation_; }

    //==============================================================================
    // Editing Settings
    //==============================================================================
    void setDefaultCrossfadeMs(int ms) { if (defaultCrossfadeMs_ != ms) { defaultCrossfadeMs_ = ms; save(); sendChangeMessage(); } }
    int getDefaultCrossfadeMs() const { return defaultCrossfadeMs_; }

    void setSnapToGrid(bool enabled) { if (snapToGrid_ != enabled) { snapToGrid_ = enabled; save(); sendChangeMessage(); } }
    bool getSnapToGrid() const { return snapToGrid_; }

    void setLinkTrackAndEditSelection(bool enabled) { if (linkTrackAndEditSelection_ != enabled) { linkTrackAndEditSelection_ = enabled; save(); sendChangeMessage(); } }
    bool getLinkTrackAndEditSelection() const { return linkTrackAndEditSelection_; }

    //==============================================================================
    // Project Settings
    //==============================================================================
    void setAutoSaveEnabled(bool enabled) { if (autoSaveEnabled_ != enabled) { autoSaveEnabled_ = enabled; save(); sendChangeMessage(); } }
    bool getAutoSaveEnabled() const { return autoSaveEnabled_; }

    void setAutoSaveIntervalMinutes(int mins) { if (autoSaveIntervalMinutes_ != mins) { autoSaveIntervalMinutes_ = mins; save(); sendChangeMessage(); } }
    int getAutoSaveIntervalMinutes() const { return autoSaveIntervalMinutes_; }

    void setMaxUndoHistory(int max) { if (maxUndoHistory_ != max) { maxUndoHistory_ = max; save(); sendChangeMessage(); } }
    int getMaxUndoHistory() const { return maxUndoHistory_; }

    void setDefaultProjectFolder(const juce::String& folder) { if (defaultProjectFolder_ != folder) { defaultProjectFolder_ = folder; save(); sendChangeMessage(); } }
    juce::String getDefaultProjectFolder() const { return defaultProjectFolder_; }

    //==============================================================================
    // Metering Settings
    //==============================================================================
    void setMeterBallistics(MeterBallistics ballistics) { if (meterBallistics_ != ballistics) { meterBallistics_ = ballistics; save(); sendChangeMessage(); } }
    MeterBallistics getMeterBallistics() const { return meterBallistics_; }

    void setMeterPeakHoldSeconds(float secs) { if (meterPeakHoldSeconds_ != secs) { meterPeakHoldSeconds_ = secs; save(); sendChangeMessage(); } }
    float getMeterPeakHoldSeconds() const { return meterPeakHoldSeconds_; }

    void setShowVolumeInDB(bool enabled) { if (showVolumeInDB_ != enabled) { showVolumeInDB_ = enabled; save(); sendChangeMessage(); } }

    bool getShowVolumeInDB() const { return showVolumeInDB_; }

    //==============================================================================
    // Zenith Hub Settings
    //==============================================================================
    void setCustomGreeting(const juce::String& greeting) { if (customGreeting_ != greeting) { customGreeting_ = greeting; save(); sendChangeMessage(); } }
    juce::String getCustomGreeting() const { return customGreeting_; }

    //==============================================================================
    // Power Management
    //==============================================================================
    void setStayAwakeDuringProject(bool stayAwake) { if (stayAwakeDuringProject_ != stayAwake) { stayAwakeDuringProject_ = stayAwake; save(); sendChangeMessage(); } }
    bool getStayAwakeDuringProject() const { return stayAwakeDuringProject_; }

    //==============================================================================
    // Plugin Settings
    //==============================================================================
    // Managed by PluginHost

private:
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

    // Zenith Hub
    juce::String customGreeting_;

    // Features
    bool stayAwakeDuringProject_ = true;
};

} // namespace zenith