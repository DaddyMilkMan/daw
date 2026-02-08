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
#include "AudioDeviceManager.h"
#include <atomic>
#include <exception>

namespace zenith {

// Concrete audio device management implementation


AudioDeviceManager::AudioDeviceManager() {
    DBG("AudioDeviceManager: Constructor");
}

AudioDeviceManager::~AudioDeviceManager() {
    DBG("AudioDeviceManager: Destructor");
    shutdown();
}

bool AudioDeviceManager::initialize(double sampleRate, int bufferSize) {
    if (initialized_.load()) {
        DBG("AudioDeviceManager: Already initialized");
        return true;
    }

    DBG("AudioDeviceManager: Initializing with sampleRate=" + juce::String(sampleRate) +
        ", bufferSize=" + juce::String(bufferSize));

    try {
        // Set up audio device settings
        deviceManager_.initialiseWithDefaultDevices(0, 2); // 0 inputs, 2 outputs
        auto* device = deviceManager_.getCurrentAudioDevice();

        if (device == nullptr) {
            DBG("AudioDeviceManager: Failed to get current audio device");
            return false;
        }

        deviceManager_.addAudioCallback(this);

        currentDeviceInfo_ = device->getName() + " (" +
                           juce::String(device->getCurrentSampleRate()) + " Hz, " +
                           juce::String(device->getCurrentBufferSizeSamples()) + " samples)";

        DBG("AudioDeviceManager: Device initialized - " + currentDeviceInfo_);
        initialized_.store(true);
        return true;
    }
    catch (const std::exception& e) {
        DBG("AudioDeviceManager: Initialization failed - " + juce::String(e.what()));
        return false;
    }
}

void AudioDeviceManager::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    DBG("AudioDeviceManager: Shutting down");

    deviceManager_.removeAudioCallback(this);
    deviceManager_.closeAudioDevice();
    currentDevice_.reset();
    initialized_.store(false);
    suspended_.store(false);

    DBG("AudioDeviceManager: Shutdown complete");
}

bool AudioDeviceManager::isInitialized() const {
    return initialized_.load();
}

juce::String AudioDeviceManager::getDeviceInfo() const {
    return currentDeviceInfo_;
}

void AudioDeviceManager::setSuspended(bool suspended) {
    suspended_.store(suspended);
}

bool AudioDeviceManager::isSuspended() const {
    return suspended_.load();
}

void AudioDeviceManager::setCallbackEnabled(bool enabled) {
    callbackEnabled_.store(enabled);
}

void AudioDeviceManager::setAudioCallback(AudioCallback callback) {
    audioCallback_ = callback;
}

double AudioDeviceManager::getCpuUsage() const {
    return cpuUsage_.load();
}

juce::int64 AudioDeviceManager::getLastCallbackTime() const {
    return lastCallbackTime_.load();
}

juce::AudioDeviceManager& AudioDeviceManager::getDeviceManager() {
    return deviceManager_;
}

const juce::AudioDeviceManager& AudioDeviceManager::getDeviceManager() const {
    return deviceManager_;
}

void AudioDeviceManager::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    DBG("AudioDeviceManager: Device about to start - " + device->getName());
    // Don't reset the device pointer - we don't own it
    currentDeviceInfo_ = device->getName() + " (" +
                        juce::String(device->getCurrentSampleRate()) + " Hz, " +
                        juce::String(device->getCurrentBufferSizeSamples()) + " samples)";
}

void AudioDeviceManager::audioDeviceStopped() {
    DBG("AudioDeviceManager: Device stopped");
    currentDevice_.reset();
    currentDeviceInfo_.clear();
}

void AudioDeviceManager::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context) noexcept {

    // Update callback time for CPU usage calculation
    lastCallbackTime_.store(juce::Time::getMillisecondCounterHiRes());

    // If suspended, silence output and return
    if (suspended_.load() || !callbackEnabled_.load()) {
        for (int channel = 0; channel < numOutputChannels; ++channel) {
            if (outputChannelData[channel] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
        return;
    }

    if (audioCallback_) {
        audioCallback_(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples, context);
    } else {
        // Clear output if no callback
        for (int channel = 0; channel < numOutputChannels; ++channel) {
            if (outputChannelData[channel] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
    }

    // Update CPU usage
    updateCpuUsage();
}

void AudioDeviceManager::handleIncomingMidiMessage(
    juce::MidiInput* source,
    const juce::MidiMessage& message) {
    // TODO: Route MIDI to appropriate tracks
    DBG("AudioDeviceManager: MIDI message received - " + juce::String(message.getRawDataSize()) + " bytes");
}

void AudioDeviceManager::updateCpuUsage() {
    // Simple CPU usage calculation based on callback timing
    auto now = juce::Time::getMillisecondCounterHiRes();
    auto lastCheck = lastCallbackTime_.load();

    if (lastCheck > 0) {
        auto timeDiff = now - lastCheck;
        if (timeDiff > 0) {
            // Simple heuristic - adjust based on actual processing time
            double usage = std::min(100.0, timeDiff * 0.1);
            cpuUsage_.store(usage);
        }
    }
}

} // namespace zenith