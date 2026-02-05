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

/*
    ==============================================================================
    Original file header:
*/

 * @file IAudioEngine.h
 * @brief Core audio engine interface for clean architecture
 *
 * This interface defines the contract for the audio engine,
 * separating concerns and enabling dependency injection.
 */


#include <memory>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "EngineEvent.h"

namespace zenith {

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    //==========================================================================
    // Lifecycle Management
    //==========================================================================

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    //==========================================================================
    // Transport Controls
    //==========================================================================

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual void record() = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
    virtual void toggleRecording() = 0;
    virtual void panic() = 0;

    //==========================================================================
    // Position & Looping
    //==========================================================================

    virtual juce::int64 getPlayheadSamples() const = 0;
    virtual double getPlaybackPositionBeats() const = 0;
    virtual void setPlayheadSamples(juce::int64 position) = 0;
    virtual void setLooping(bool shouldLoop) = 0;
    virtual bool isLooping() const = 0;
    virtual void setLoopRegion(juce::int64 start, juce::int64 end) = 0;
    virtual juce::int64 getLoopStart() const = 0;
    virtual juce::int64 getLoopEnd() const = 0;

    //==========================================================================
    // Audio Processing
    //==========================================================================

    virtual void suspendProcessing(bool shouldSuspend) = 0;
    virtual bool isSuspended() const = 0;
    virtual double getCpuUsage() const = 0;
    virtual double getSampleRate() const = 0;
    virtual int getBufferSize() const = 0;

    //==========================================================================
    // Device Management
    //==========================================================================

    virtual juce::AudioDeviceManager& getDeviceManager() = 0;
    virtual juce::String getAudioDeviceInfo() const = 0;

    //==========================================================================
    // Event System
    //==========================================================================

    virtual bool queueEvent(const EngineEvent& e) = 0;
    virtual void addChangeListener(juce::ChangeListener* listener) = 0;
    virtual void removeChangeListener(juce::ChangeListener* listener) = 0;

    //==========================================================================
    // Configuration
    //==========================================================================

    virtual void setEnableTestTone(bool enabled) = 0;
    virtual bool isTestToneEnabled() const = 0;
};

} // namespace zenith