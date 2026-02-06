/*
    Settings.cpp - Global application settings implementation
*/

#include "Settings.h"

namespace zenith {

void Settings::load() {
    // Basic implementation for now - could load from PropertiesFile in future
    loadDefaults();
}

void Settings::save() {
    // Basic implementation for now - could save to PropertiesFile in future
}

void Settings::loadDefaults() {
    renderBackend = SkiaRenderer::Backend::Auto;
    targetFPS = 60;
    globalScale = 1.0f;
    glowIntensity = 1.0f;
    theme = UITheme::Neon;
    animationsEnabled = true;
    highContrastMode = false;

    linuxAudioBackend = LinuxAudioBackend::Auto;
    bufferSize = 512;
    pluginDelayCompensation = true;
    softwareMonitoring = true;
    monitoringVolume = 1.0f;

    countInBars = 1;
    metronomeCountIn = true;
    recordingBitDepth = RecordingBitDepth::Bit24;
    recordingFileType = RecordingFileType::WAV;
    allowTempoChangeDuringRecord = false;

    midiThrough = true;
    sendMIDIClockOut = false;
    receiveMTCIn = false;
    midiLatencyCompensation = 0;

    defaultCrossfadeMs = 10;
    snapToGrid = true;
    linkTrackAndEditSelection = true;

    autoSaveEnabled = true;
    autoSaveIntervalMinutes = 5;
    maxUndoHistory = 100;

    meterBallistics = MeterBallistics::Peak;
    meterPeakHoldSeconds = 2.0f;
    showVolumeInDB = true;

    aiModelMode = AIModelMode::Obedient;
    wingmanReasoningDisplay = false;
    wingmanWebSearch = false;
    wingmanStreaming = true;
    wingmanDAWContext = true;
    wingmanMaxHistory = 10;

    stayAwakeDuringProject = true;
    audioSampleRate = 44100;
    audioChannels = 2;
}

} // namespace zenith