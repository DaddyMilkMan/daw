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

 // File: AudioEngineCore.h
 // Brief: Core audio engine interface and basic functionality
 * 
 * This is the simplified core engine that handles:
 * - Audio device I/O
 * - Basic transport control
 * - Audio processing callback interface
 * - MIDI input callback
 */



#pragma once

#include <atomic>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <memory>
#include <vector>

namespace zenith {

// Forward declarations
class ProjectState;

//==============================================================================
/**
 * @class AudioEngineCore
 // Brief: Core audio engine interface
 * 
 * This class provides the fundamental audio engine functionality without
 * the complexity of track management, mixing, or advanced features.
 * It's designed to be RT-safe and minimal.
 */
class AudioEngineCore : public juce::AudioIODeviceCallback,
                       public juce::MidiInputCallback,
                       public juce::ChangeListener {
public:
    //==========================================================================
    AudioEngineCore();
    virtual ~AudioEngineCore();

    //==========================================================================
    // Basic Initialization
    //==========================================================================

    /**
     // Brief: Initialize the audio engine
     * @param sampleRate Desired sample rate
     * @param bufferSize Desired buffer size
     * @return True if initialization successful
     */
    virtual bool initialize(double sampleRate = 48000, int bufferSize = 512);

    /**
     // Brief: Shutdown the audio engine
     */
    virtual void shutdown();

    //==========================================================================
    // Device Management
    //==========================================================================

    /**
     // Brief: Get the audio device manager
     */
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    /**
     // Brief: Get the plugin format manager
     */
    juce::AudioPluginFormatManager& getPluginFormatManager();

    //==========================================================================
    // Transport Control
    //==========================================================================

    /**
     // Brief: Start playback
     */
    virtual void startPlayback();

    /**
     // Brief: Stop playback
     */
    virtual void stopPlayback();

    /**
     // Brief: Check if currently playing
     */
    virtual bool isPlaying() const { return isPlaying_.load(); }

    /**
     // Brief: Set playback position
     * @param positionInSamples Position in samples
     */
    virtual void setPlaybackPosition(juce::int64 positionInSamples);

    /**
     // Brief: Get current playback position
     */
    virtual juce::int64 getPlaybackPosition() const { return currentPosition_.load(); }

    //==========================================================================
    // Project Integration
    //==========================================================================

    /**
     // Brief: Set project state for tempo and automation
     * @param state Pointer to project state
     */
    virtual void setProjectState(ProjectState* state);

    /**
     // Brief: Get current sample rate
     */
    double getSampleRate() const { return currentSampleRate_; }

    /**
     // Brief: Get current buffer size
     */
    int getBufferSize() const { return currentBufferSize_; }

    //==========================================================================
    // AudioIODeviceCallback Interface
    //==========================================================================

    void audioDeviceIOCallback(const float** inputChannelData,
                              int numInputChannels,
                              float** outputChannelData,
                              int numOutputChannels,
                              int numSamples) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    //==========================================================================
    // MidiInputCallback Interface
    //==========================================================================

    void handleIncomingMidiMessage(juce::MidiInput* source,
                                  const juce::MidiMessage& message) override;

    //==========================================================================
    // ChangeListener Interface
    //==========================================================================

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

protected:
    //==========================================================================
    // Processing Hooks (for subclasses)
    //==========================================================================

    /**
     // Brief: Process audio - override in subclasses
     */
    virtual void processAudio(const float** inputChannels,
                             float** outputChannels,
                             int numInputChannels,
                             int numOutputChannels,
                             int numSamples);

    /**
     // Brief: Handle MIDI message - override in subclasses
     */
    virtual void processMidi(const juce::MidiMessage& message);

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Device management
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<juce::AudioPluginFormatManager> pluginFormatManager;

    // Transport state (atomic for thread safety)
    std::atomic<bool> isPlaying_{false};
    std::atomic<juce::int64> currentPosition_{0};

    // Audio parameters
    double currentSampleRate_ = 48000.0;
    int currentBufferSize_ = 512;

    // Project integration
    ProjectState* projectState_ = nullptr;

    // CPU monitoring
    std::atomic<float> cpuUsage_{0.0f};
    juce::int64 lastProcessTime_ = 0;

private:
    //==========================================================================
    // Internal Methods
    //==========================================================================

    void updateCPUUsage(int numSamples);
    void initializeAudioFormats();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngineCore)
};

} // namespace zenith
