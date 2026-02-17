/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <chrono>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace zenith {
namespace audio {

// Lock-free ring buffer for real-time audio
template<typename T, size_t Size>
class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    // Device management
    bool initialize(double sampleRate = 44100.0, int bufferSize = 512);
    void shutdown();

    // Device selection
    std::vector<juce::String> getAvailableInputDevices() const;
    std::vector<juce::String> getAvailableOutputDevices() const;
    bool setInputDevice(const juce::String& deviceName);
    bool setOutputDevice(const juce::String& deviceName);

    // Configuration
    bool setSampleRate(double sampleRate);
    bool setBufferSize(int bufferSize);
    double getCurrentSampleRate() const;
    int getCurrentBufferSize() const;

    // Real-time audio callback
    void setAudioCallback(juce::AudioIODeviceCallback* callback);
    void removeAudioCallback();

    // Monitoring
    float getInputLevel(int channel) const;
    float getOutputLevel(int channel) const;
    bool isDeviceActive() const;

    // Error handling
    juce::String getLastError() const;
    bool hasErrors() const;

private:
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    // OWNERSHIP: NON-OWNING pointer to device managed by juce::AudioDeviceManager
    juce::AudioIODevice* currentDevice = nullptr;
    juce::String lastError;

    // Device state
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    bool isActive = false;

    // Level monitoring
    std::vector<float> inputLevels;
    std::vector<float> outputLevels;

    bool selectBestDevice();
    void updateDeviceList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceManager)
};

// Sample rate converter with high quality

} // namespace
