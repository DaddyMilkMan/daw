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

#include <functional>
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

class IAudioDeviceManager : public juce::AudioIODeviceCallback,
                          public juce::MidiInputCallback {
public:
    virtual ~IAudioDeviceManager() = default;

    //==========================================================================
    // Device Management
    //==========================================================================

    virtual bool initialize(double sampleRate, int bufferSize) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    virtual juce::String getDeviceInfo() const = 0;

    //==========================================================================
    // Audio Processing
    //==========================================================================

    virtual void setSuspended(bool suspended) = 0;
    virtual bool isSuspended() const = 0;
    virtual void setCallbackEnabled(bool enabled) = 0;

    //==========================================================================
    // Statistics
    //==========================================================================

    virtual double getCpuUsage() const = 0;
    virtual juce::int64 getLastCallbackTime() const = 0;

    //==========================================================================
    // Device Access
    //==========================================================================

    virtual juce::AudioDeviceManager& getDeviceManager() = 0;
    virtual const juce::AudioDeviceManager& getDeviceManager() const = 0;

    // Interface implementations
    virtual void audioDeviceAboutToStart(juce::AudioIODevice* device) override = 0;
    virtual void audioDeviceStopped() override = 0;
    virtual void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) noexcept override = 0;

    virtual void handleIncomingMidiMessage(
        juce::MidiInput* source,
        const juce::MidiMessage& message) override = 0;
};

} // namespace zenith