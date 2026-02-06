/*
    Settings.h - Global application settings
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "rendering/SkiaRenderer.h"

namespace zenith {

using SkiaRendererTest = SkiaRenderer;

class Settings : public juce::ChangeBroadcaster {
public:
    enum class LinuxAudioBackend { Auto, JACK, PipeWire, ALSA };
    enum class RecordingBitDepth { Bit16, Bit24, Bit32Float };
    enum class RecordingFileType { WAV, AIFF, FLAC };
    enum class MeterBallistics { VU, PPM, Peak };
    enum class UITheme { Neon, Dark, Light };
    enum class AIModelMode { Obedient, Teacher, Creative };

    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    void load();
    void save();

    // Display
    void setRenderBackend(SkiaRenderer::Backend b) { if (renderBackend != b) { renderBackend = b; save(); sendChangeMessage(); } }
    SkiaRenderer::Backend getRenderBackend() const { return renderBackend; }

    void setTargetFPS(int f) { if (targetFPS != f) { targetFPS = f; save(); sendChangeMessage(); } }
    int getTargetFPS() const { return targetFPS; }

    void setGlobalScale(float s) { if (globalScale != s) { globalScale = s; save(); sendChangeMessage(); } }
    float getGlobalScale() const { return globalScale; }

    void setGlowIntensity(float i) { if (glowIntensity != i) { glowIntensity = i; save(); sendChangeMessage(); } }
    float getGlowIntensity() const { return glowIntensity; }

    void setTheme(UITheme t) { if (theme != t) { theme = t; save(); sendChangeMessage(); } }
    UITheme getTheme() const { return theme; }

    void setAnimationsEnabled(bool e) { if (animationsEnabled != e) { animationsEnabled = e; save(); sendChangeMessage(); } }
    bool getAnimationsEnabled() const { return animationsEnabled; }

    void setHighContrastMode(bool e) { if (highContrastMode != e) { highContrastMode = e; save(); sendChangeMessage(); } }
    bool getHighContrastMode() const { return highContrastMode; }

    // Audio
    void setLinuxAudioBackend(LinuxAudioBackend b) { if (linuxAudioBackend != b) { linuxAudioBackend = b; save(); sendChangeMessage(); } }
    LinuxAudioBackend getLinuxAudioBackend() const { return linuxAudioBackend; }

    void setBufferSize(int s) { if (bufferSize != s) { bufferSize = s; save(); sendChangeMessage(); } }
    int getBufferSize() const { return bufferSize; }

    void setPluginDelayCompensation(bool e) { if (pluginDelayCompensation != e) { pluginDelayCompensation = e; save(); sendChangeMessage(); } }
    bool getPluginDelayCompensation() const { return pluginDelayCompensation; }

    void setSoftwareMonitoring(bool e) { if (softwareMonitoring != e) { softwareMonitoring = e; save(); sendChangeMessage(); } }
    bool getSoftwareMonitoring() const { return softwareMonitoring; }

    void setMonitoringVolume(float v) { if (monitoringVolume != v) { monitoringVolume = v; save(); sendChangeMessage(); } }
    float getMonitoringVolume() const { return monitoringVolume; }

    // Recording
    void setCountInBars(int b) { if (countInBars != b) { countInBars = b; save(); sendChangeMessage(); } }
    int getCountInBars() const { return countInBars; }

    void setMetronomeCountIn(bool e) { if (metronomeCountIn != e) { metronomeCountIn = e; save(); sendChangeMessage(); } }
    bool getMetronomeCountIn() const { return metronomeCountIn; }

    void setRecordingBitDepth(RecordingBitDepth d) { if (recordingBitDepth != d) { recordingBitDepth = d; save(); sendChangeMessage(); } }
    RecordingBitDepth getRecordingBitDepth() const { return recordingBitDepth; }

    void setRecordingFileType(RecordingFileType t) { if (recordingFileType != t) { recordingFileType = t; save(); sendChangeMessage(); } }
    RecordingFileType getRecordingFileType() const { return recordingFileType; }

    void setAllowTempoChangeDuringRecord(bool a) { if (allowTempoChangeDuringRecord != a) { allowTempoChangeDuringRecord = a; save(); sendChangeMessage(); } }
    bool getAllowTempoChangeDuringRecord() const { return allowTempoChangeDuringRecord; }

    // Audio settings (missing from original Settings.h)
    void setAudioSampleRate(int rate) { if (audioSampleRate != rate) { audioSampleRate = rate; save(); sendChangeMessage(); } }
    int getAudioSampleRate() const { return audioSampleRate; }
    void setAudioChannels(int channels) { if (audioChannels != channels) { audioChannels = channels; save(); sendChangeMessage(); } }
    int getAudioChannels() const { return audioChannels; }

    // MIDI
    void setMIDIThrough(bool e) { if (midiThrough != e) { midiThrough = e; save(); sendChangeMessage(); } }
    bool getMIDIThrough() const { return midiThrough; }

    void setSendMIDIClockOut(bool e) { if (sendMIDIClockOut != e) { sendMIDIClockOut = e; save(); sendChangeMessage(); } }
    bool getSendMIDIClockOut() const { return sendMIDIClockOut; }

    void setReceiveMTCIn(bool e) { if (receiveMTCIn != e) { receiveMTCIn = e; save(); sendChangeMessage(); } }
    bool getReceiveMTCIn() const { return receiveMTCIn; }

    void setMIDILatencyCompensation(int m) { if (midiLatencyCompensation != m) { midiLatencyCompensation = m; save(); sendChangeMessage(); } }
    int getMIDILatencyCompensation() const { return midiLatencyCompensation; }

    // Editing
    void setDefaultCrossfadeMs(int m) { if (defaultCrossfadeMs != m) { defaultCrossfadeMs = m; save(); sendChangeMessage(); } }
    int getDefaultCrossfadeMs() const { return defaultCrossfadeMs; }

    void setSnapToGrid(bool e) { if (snapToGrid != e) { snapToGrid = e; save(); sendChangeMessage(); } }
    bool getSnapToGrid() const { return snapToGrid; }

    void setLinkTrackAndEditSelection(bool e) { if (linkTrackAndEditSelection != e) { linkTrackAndEditSelection = e; save(); sendChangeMessage(); } }
    bool getLinkTrackAndEditSelection() const { return linkTrackAndEditSelection; }

    // Project
    void setAutoSaveEnabled(bool e) { if (autoSaveEnabled != e) { autoSaveEnabled = e; save(); sendChangeMessage(); } }
    bool getAutoSaveEnabled() const { return autoSaveEnabled; }

    void setAutoSaveIntervalMinutes(int m) { if (autoSaveIntervalMinutes != m) { autoSaveIntervalMinutes = m; save(); sendChangeMessage(); } }
    int getAutoSaveIntervalMinutes() const { return autoSaveIntervalMinutes; }

    void setMaxUndoHistory(int m) { if (maxUndoHistory != m) { maxUndoHistory = m; save(); sendChangeMessage(); } }
    int getMaxUndoHistory() const { return maxUndoHistory; }

    void setDefaultProjectFolder(const juce::String& f) { if (defaultProjectFolder != f) { defaultProjectFolder = f; save(); sendChangeMessage(); } }
    juce::String getDefaultProjectFolder() const { return defaultProjectFolder; }

    // Metering
    void setMeterBallistics(MeterBallistics b) { if (meterBallistics != b) { meterBallistics = b; save(); sendChangeMessage(); } }
    MeterBallistics getMeterBallistics() const { return meterBallistics; }

    void setMeterPeakHoldSeconds(float s) { if (meterPeakHoldSeconds != s) { meterPeakHoldSeconds = s; save(); sendChangeMessage(); } }
    float getMeterPeakHoldSeconds() const { return meterPeakHoldSeconds; }

    void setShowVolumeInDB(bool e) { if (showVolumeInDB != e) { showVolumeInDB = e; save(); sendChangeMessage(); } }
    bool getShowVolumeInDB() const { return showVolumeInDB; }

    // Wingman AI Settings
    void setAIModelMode(AIModelMode m) { if (aiModelMode != m) { aiModelMode = m; save(); sendChangeMessage(); } }
    AIModelMode getAIModelMode() const { return aiModelMode; }

    void setWingmanReasoningDisplay(bool e) { if (wingmanReasoningDisplay != e) { wingmanReasoningDisplay = e; save(); sendChangeMessage(); } }
    bool getWingmanReasoningDisplay() const { return wingmanReasoningDisplay; }

    void setWingmanWebSearch(bool e) { if (wingmanWebSearch != e) { wingmanWebSearch = e; save(); sendChangeMessage(); } }
    bool getWingmanWebSearch() const { return wingmanWebSearch; }

    void setWingmanStreaming(bool e) { if (wingmanStreaming != e) { wingmanStreaming = e; save(); sendChangeMessage(); } }
    bool getWingmanStreaming() const { return wingmanStreaming; }

    void setWingmanDAWContext(bool e) { if (wingmanDAWContext != e) { wingmanDAWContext = e; save(); sendChangeMessage(); } }
    bool getWingmanDAWContext() const { return wingmanDAWContext; }

    void setWingmanMaxHistory(int m) { if (wingmanMaxHistory != m) { wingmanMaxHistory = m; save(); sendChangeMessage(); } }
    int getWingmanMaxHistory() const { return wingmanMaxHistory; }

    // Zenith Hub
    void setCustomGreeting(const juce::String& g) { if (customGreeting != g) { customGreeting = g; save(); sendChangeMessage(); } }
    juce::String getCustomGreeting() const { return customGreeting; }

    // Features
    void setStayAwakeDuringProject(bool s) { if (stayAwakeDuringProject != s) { stayAwakeDuringProject = s; save(); sendChangeMessage(); } }
    bool getStayAwakeDuringProject() const { return stayAwakeDuringProject; }

private:
    Settings() = default;
    void sendChangeMessage() { juce::ChangeBroadcaster::sendChangeMessage(); }
    void loadDefaults(); // For initialization after construction

    SkiaRenderer::Backend renderBackend = SkiaRenderer::Backend::Auto;
    int targetFPS = 60;
    float globalScale = 1.0f;
    float glowIntensity = 1.0f;
    UITheme theme = UITheme::Neon;
    bool animationsEnabled = true;
    bool highContrastMode = false;

    LinuxAudioBackend linuxAudioBackend = LinuxAudioBackend::Auto;
    int bufferSize = 512;
    bool pluginDelayCompensation = true;
    bool softwareMonitoring = true;
    float monitoringVolume = 1.0f;

    int countInBars = 1;
    bool metronomeCountIn = true;
    RecordingBitDepth recordingBitDepth = RecordingBitDepth::Bit24;
    RecordingFileType recordingFileType = RecordingFileType::WAV;
    bool allowTempoChangeDuringRecord = false;

    bool midiThrough = true;
    bool sendMIDIClockOut = false;
    bool receiveMTCIn = false;
    int midiLatencyCompensation = 0;

    int defaultCrossfadeMs = 10;
    bool snapToGrid = true;
    bool linkTrackAndEditSelection = true;

    bool autoSaveEnabled = true;
    int autoSaveIntervalMinutes = 5;
    int maxUndoHistory = 100;
    juce::String defaultProjectFolder;

    MeterBallistics meterBallistics = MeterBallistics::Peak;
    float meterPeakHoldSeconds = 2.0f;
    bool showVolumeInDB = true;

    AIModelMode aiModelMode = AIModelMode::Obedient;
    bool wingmanReasoningDisplay = false;
    bool wingmanWebSearch = false;
    bool wingmanStreaming = true;
    bool wingmanDAWContext = true;
    int wingmanMaxHistory = 10;

    juce::String customGreeting;
    bool stayAwakeDuringProject = true;

    // Audio settings (missing from original Settings.h)
    int audioSampleRate = 44100;
    int audioChannels = 2;
};

} // namespace zenith
