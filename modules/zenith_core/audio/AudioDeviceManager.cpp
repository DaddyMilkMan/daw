/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "AudioDeviceManager.h"

namespace zenith {
namespace audio {

//==============================================================================
AudioDeviceManager::AudioDeviceManager()
    : deviceManager(std::make_unique<juce::AudioDeviceManager>())
{
}

AudioDeviceManager::~AudioDeviceManager()
{
    shutdown();
}

//==============================================================================
bool AudioDeviceManager::initialize(double sampleRate, int bufferSize)
{
    lastError.clear();

    // Initialize the JUCE device manager
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.sampleRate = sampleRate;
    setup.bufferSize = bufferSize;

    auto result = deviceManager->initialise(
        0,                      // Default input channels
        2,                      // Default output channels (stereo)
        nullptr,                // No preferred device
        true                    // Select default device
    );

    if (result.failed())
    {
        lastError = result.getErrorMessage();
        return false;
    }

    // Get current device and configuration
    currentDevice = deviceManager->getCurrentAudioDevice();
    if (currentDevice == nullptr)
    {
        lastError = "Failed to get current audio device";
        return false;
    }

    currentSampleRate = currentDevice->getCurrentSampleRate();
    currentBufferSize = currentDevice->getCurrentBufferSizeSamples();
    isActive = true;

    // Initialize level monitoring arrays
    auto numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
    auto numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();

    inputLevels.resize(numInputChannels, 0.0f);
    outputLevels.resize(numOutputChannels, 0.0f);

    return true;
}

void AudioDeviceManager::shutdown()
{
    removeAudioCallback();
    deviceManager->closeAudioDevice();
    currentDevice = nullptr;
    isActive = false;
    inputLevels.clear();
    outputLevels.clear();
}

//==============================================================================
std::vector<juce::String> AudioDeviceManager::getAvailableInputDevices() const
{
    std::vector<juce::String> devices;
    auto deviceTypes = deviceManager->getAvailableDeviceTypes();

    for (auto* type : deviceTypes)
    {
        auto deviceNames = type->getDeviceNames();
        for (auto& name : deviceNames)
        {
            devices.push_back(name);
        }
    }

    return devices;
}

std::vector<juce::String> AudioDeviceManager::getAvailableOutputDevices() const
{
    // Most audio devices have both input and output
    return getAvailableInputDevices();
}

bool AudioDeviceManager::setInputDevice(const juce::String& deviceName)
{
    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager->getAudioDeviceSetup();

    // Find the device type
    auto deviceTypes = deviceManager->getAvailableDeviceTypes();
    for (auto* type : deviceTypes)
    {
        auto deviceNames = type->getDeviceNames();
        for (int i = 0; i < deviceNames.size(); ++i)
        {
            if (deviceNames[i] == deviceName)
            {
                setup.inputDeviceName = deviceName;
                auto result = deviceManager->setAudioDeviceSetup(setup, true);

                if (result.failed())
                {
                    lastError = result.getErrorMessage();
                    return false;
                }

                currentDevice = deviceManager->getCurrentAudioDevice();
                updateDeviceList();
                return true;
            }
        }
    }

    lastError = "Device not found: " + deviceName;
    return false;
}

bool AudioDeviceManager::setOutputDevice(const juce::String& deviceName)
{
    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager->getAudioDeviceSetup();

    // Find the device type
    auto deviceTypes = deviceManager->getAvailableDeviceTypes();
    for (auto* type : deviceTypes)
    {
        auto deviceNames = type->getDeviceNames();
        for (int i = 0; i < deviceNames.size(); ++i)
        {
            if (deviceNames[i] == deviceName)
            {
                setup.outputDeviceName = deviceName;
                auto result = deviceManager->setAudioDeviceSetup(setup, true);

                if (result.failed())
                {
                    lastError = result.getErrorMessage();
                    return false;
                }

                currentDevice = deviceManager->getCurrentAudioDevice();
                updateDeviceList();
                return true;
            }
        }
    }

    lastError = "Device not found: " + deviceName;
    return false;
}

//==============================================================================
bool AudioDeviceManager::setSampleRate(double sampleRate)
{
    if (!isActive || !currentDevice)
        return false;

    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager->getAudioDeviceSetup();
    setup.sampleRate = sampleRate;

    auto result = deviceManager->setAudioDeviceSetup(setup, true);
    if (result.failed())
    {
        lastError = result.getErrorMessage();
        return false;
    }

    currentSampleRate = sampleRate;
    return true;
}

bool AudioDeviceManager::setBufferSize(int bufferSize)
{
    if (!isActive || !currentDevice)
        return false;

    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager->getAudioDeviceSetup();
    setup.bufferSize = bufferSize;

    auto result = deviceManager->setAudioDeviceSetup(setup, true);
    if (result.failed())
    {
        lastError = result.getErrorMessage();
        return false;
    }

    currentBufferSize = bufferSize;
    return true;
}

double AudioDeviceManager::getCurrentSampleRate() const
{
    if (currentDevice)
        return currentDevice->getCurrentSampleRate();
    return currentSampleRate;
}

int AudioDeviceManager::getCurrentBufferSize() const
{
    if (currentDevice)
        return currentDevice->getCurrentBufferSizeSamples();
    return currentBufferSize;
}

//==============================================================================
void AudioDeviceManager::setAudioCallback(juce::AudioIODeviceCallback* callback)
{
    if (callback)
    {
        deviceManager->addAudioCallback(callback);
    }
}

void AudioDeviceManager::removeAudioCallback()
{
    deviceManager->removeAudioCallback(nullptr);
}

//==============================================================================
float AudioDeviceManager::getInputLevel(int channel) const
{
    if (channel >= 0 && channel < static_cast<int>(inputLevels.size()))
        return inputLevels[channel];
    return 0.0f;
}

float AudioDeviceManager::getOutputLevel(int channel) const
{
    if (channel >= 0 && channel < static_cast<int>(outputLevels.size()))
        return outputLevels[channel];
    return 0.0f;
}

bool AudioDeviceManager::isDeviceActive() const
{
    return isActive && currentDevice != nullptr;
}

//==============================================================================
juce::String AudioDeviceManager::getLastError() const
{
    return lastError;
}

bool AudioDeviceManager::hasErrors() const
{
    return !lastError.isEmpty();
}

//==============================================================================
bool AudioDeviceManager::selectBestDevice()
{
    // Try to use the default device
    auto result = deviceManager->initialise(
        0,          // Default input channels
        2,          // Default output channels
        nullptr,    // No XML settings
        true        // Select default device
    );

    if (result.failed())
    {
        lastError = result.getErrorMessage();
        return false;
    }

    currentDevice = deviceManager->getCurrentAudioDevice();
    return currentDevice != nullptr;
}

void AudioDeviceManager::updateDeviceList()
{
    // Device list is updated automatically by JUCE
    // This method can be used for custom device management
}

} // namespace audio
} // namespace zenith
