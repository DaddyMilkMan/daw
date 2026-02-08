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

#pragma once

//==============================================================================

#include "IAudioDeviceManager.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <memory>

namespace zenith {

class AudioDeviceManager : public IAudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager() override;

    //==========================================================================
    // IAudioDeviceManager Implementation
    //==========================================================================

    bool initialize(double sampleRate, int bufferSize) override;
    void shutdown() override;
    bool isInitialized() const override;
    juce::String getDeviceInfo() const override;

    void setAudioCallback(AudioCallback callback) override;
    void setSuspended(bool suspended) override;
    bool isSuspended() const override;
    void setCallbackEnabled(bool enabled) override;

    double getCpuUsage() const override;
    juce::int64 getLastCallbackTime() const override;

    juce::AudioDeviceManager& getDeviceManager() override;
    const juce::AudioDeviceManager& getDeviceManager() const override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) noexcept override;

    void handleIncomingMidiMessage(
        juce::MidiInput* source,
        const juce::MidiMessage& message) override;

private:
    //==========================================================================
    // Internal State
    //==========================================================================

    std::atomic<bool> initialized_{false};
    std::atomic<bool> suspended_{false};
    std::atomic<bool> callbackEnabled_{true};
    mutable std::atomic<double> cpuUsage_{0.0};
    std::atomic<juce::int64> lastCallbackTime_{0};

    //==========================================================================
    // Device Management
    //==========================================================================

    juce::AudioDeviceManager deviceManager_;
    std::unique_ptr<juce::AudioIODevice> currentDevice_;
    juce::String currentDeviceInfo_;

    //==========================================================================
    // Audio Processing
    //==========================================================================

    void updateCpuUsage();
    AudioCallback audioCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceManager)
};

} // namespace zenith