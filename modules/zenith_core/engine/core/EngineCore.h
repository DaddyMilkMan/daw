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

/**
 * @file EngineCore.h
 * @brief Refactored engine core with clean architecture
 *
 * This is the new, clean Engine implementation that uses dependency injection
 * and follows proper architectural boundaries.
 */


#include "IAudioEngine.h"
#include "IAudioDeviceManager.h"
#include "ITrackManager.h"
#include "ITransportController.h"
#include "IAudioRenderer.h"
#include "../RecordingManager.h"
#include <memory>
#include <functional>
#include <juce_core/juce_core.h>

namespace zenith {

class ProjectState;
class InstrumentRegistry;
class PluginHost;
class AudioFilePool;
class Metronome;
class Midi2DiscoveryService;
class SessionDebuggerAgent;
class AIMasteringAgent;

// Forward declarations for incomplete types that are only used as pointers
namespace zenith {
class SessionDebuggerAgent;
class AIMasteringAgent;
}

/**
 * @class EngineCore
 * @brief Refactored engine core with modular architecture
 *
 * This Engine class is now a clean orchestrator that delegates to specialized components:
 * - AudioDeviceManager: Handles audio device I/O
 * - TrackManager: Manages track lifecycle and state
 * - TransportController: Controls transport and position
 * - AudioRenderer: Handles audio processing
 * - RecordingManager: Handles recording functionality
 * - InstrumentRegistry: Manages virtual instruments
 * - PluginHost: Manages audio plugins
 * - AudioFilePool: Manages audio file caching
 * - MIDI services: Handle MIDI input and discovery
 * - AI agents: Handle AI features
 */
class EngineCore : public IAudioEngine {
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    EngineCore();
    ~EngineCore() override;

    //==========================================================================
    // IAudioEngine Implementation
    //==========================================================================

    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override;

    // Transport Controls
    void play() override;
    void stop() override;
    bool isPlaying() const override;
    void record() override;
    void stopRecording() override;
    bool isRecording() const override;
    void toggleRecording() override;
    void panic() override;

    // Position & Looping
    juce::int64 getPlayheadSamples() const override;
    double getPlaybackPositionBeats() const override;
    void setPlayheadSamples(juce::int64 position) override;
    void setLooping(bool shouldLoop) override;
    bool isLooping() const override;
    void setLoopRegion(juce::int64 start, juce::int64 end) override;
    juce::int64 getLoopStart() const override;
    juce::int64 getLoopEnd() const override;

    // Audio Processing
    void suspendProcessing(bool shouldSuspend) override;
    bool isSuspended() const override;
    double getCpuUsage() const override;
    double getSampleRate() const override;
    int getBufferSize() const override;

    // Device Management
    juce::AudioDeviceManager& getDeviceManager() override;
    juce::String getAudioDeviceInfo() const override;

    // Event System
    bool queueEvent(const EngineEvent& e) override;
    void addChangeListener(juce::ChangeListener* listener) override;
    void removeChangeListener(juce::ChangeListener* listener) override;

    // Configuration
    void setEnableTestTone(bool enabled) override;
    bool isTestToneEnabled() const override;

    //==========================================================================
    // Component Accessors
    //==========================================================================

    // Core components
    IAudioDeviceManager& getAudioDeviceManager() { return *audioDeviceManager_; }
    ITrackManager& getTrackManager() { return *trackManager_; }
    ITransportController& getTransportController() { return *transportController_; }
    IAudioRenderer& getAudioRenderer() { return *audioRenderer_; }

    // Service components
    InstrumentRegistry& getInstrumentRegistry() { return *instrumentRegistry_; }
    PluginHost& getPluginHost() { return *pluginHost_; }
    AudioFilePool& getAudioFilePool() { return *audioFilePool_; }
    RecordingManager& getRecordingManager() { return *recordingManager_; }
    Metronome& getMetronome() { return *metronome_; }
    Midi2DiscoveryService& getMidi2DiscoveryService() { return *midi2DiscoveryService_; }

    // TODO: Fix incomplete type issues
    // AI components
    // SessionDebuggerAgent* getSessionDebugger() { return sessionDebugger_.get(); }
    // AIMasteringAgent* getAIMasteringAgent() { return masteringAgent_.get(); }

    //==========================================================================
    // Project State Management
    //==========================================================================

    void setProjectState(ProjectState* state);
    ProjectState* getProjectState() { return projectState_; }

    //==========================================================================
    // Component Management
    //==========================================================================

    void syncWithProjectState();
    void syncTempoMap();

    //==========================================================================
    // Audio Processing
    //==========================================================================

    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) noexcept;

    void handleIncomingMidiMessage(
        juce::MidiInput* source,
        const juce::MidiMessage& message);

private:
    //==========================================================================
    // Component Initialization
    //==========================================================================

    void initializeCoreComponents();
    void initializeAudioProcessing();
    void initializeInstrumentsAndPlugins();
    void initializeAIAgents();

    //==========================================================================
    // Processing Helpers
    //==========================================================================

    void processAudioBlock(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context);

    void processEvents();

    //==========================================================================
    // State Management
    //==========================================================================

    void notifyTransportChanged();
    void notifyPositionChanged();
    void notifyTrackStateChanged();

    //==========================================================================
    // Component Dependencies
    //==========================================================================

    std::unique_ptr<IAudioDeviceManager> audioDeviceManager_;
    std::unique_ptr<ITrackManager> trackManager_;
    std::unique_ptr<ITransportController> transportController_;
    std::unique_ptr<IAudioRenderer> audioRenderer_;

    std::unique_ptr<InstrumentRegistry> instrumentRegistry_;
    std::unique_ptr<PluginHost> pluginHost_;
    std::unique_ptr<AudioFilePool> audioFilePool_;
    std::unique_ptr<RecordingManager> recordingManager_;
    std::unique_ptr<Metronome> metronome_;
    std::unique_ptr<Midi2DiscoveryService> midi2DiscoveryService_;

    // TODO: Fix incomplete type issues
    // std::unique_ptr<SessionDebuggerAgent> sessionDebugger_;
    // std::unique_ptr<AIMasteringAgent> masteringAgent_;

    //==========================================================================
    // State
    //==========================================================================

    bool initialized_{false};
    bool suspended_{false};
    ProjectState* projectState_{nullptr};

    //==========================================================================
    // Event System
    //==========================================================================

    juce::ListenerList<juce::ChangeListener> changeListeners_;
    juce::AbstractFifo eventFifo_{1024};
    std::vector<EngineEvent> eventBuffer_{1024};

    //==========================================================================
    // Audio Settings
    //==========================================================================

    double sampleRate_{44100.0};
    int bufferSize_{512};
    juce::String audioDeviceInfo_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineCore)
};

} // namespace zenith