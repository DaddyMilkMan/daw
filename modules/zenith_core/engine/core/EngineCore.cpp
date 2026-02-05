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

 * @file EngineCore.cpp
 * @brief Refactored engine core with clean architecture
 */


#include "../../engine/AudioFilePool.h"
#include "../../engine/Metronome.h"
#include "../../engine/Midi2DiscoveryService.h"
#include "../../ai/SessionDebuggerAgent.h"
#include "../../ai/AIMasteringAgent.h"
#include "../ZenithLogger.h"
#include <algorithm>

namespace zenith {

EngineCore::EngineCore() {
    DBG("EngineCore: Constructor - Starting modular architecture");

    // Initialize core components
    initializeCoreComponents();
    initializeAudioProcessing();
    initializeInstrumentsAndPlugins();
    initializeAIAgents();

    DBG("EngineCore: Constructor complete - All components initialized");
}

EngineCore::~EngineCore() {
    DBG("EngineCore: Destructor - Shutting down gracefully");
    shutdown();
}

bool EngineCore::initialize() {
    if (initialized_) {
        DBG("EngineCore: Already initialized");
        return true;
    }

    DBG("EngineCore: Initializing audio engine");

    try {
        // Initialize audio device manager
        if (!audioDeviceManager_->initialize(sampleRate_, bufferSize_)) {
            DBG("EngineCore: Failed to initialize audio device manager");
            return false;
        }

        // Initialize audio renderer
        audioRenderer_->initialize(
            audioDeviceManager_->getDeviceManager().getCurrentAudioDevice()->getDefaultBufferSizeSamples(),
            audioDeviceManager_->getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate()
        );

        // Prepare tracks for processing
        trackManager_->prepareForProcessing(
            audioDeviceManager_->getDeviceManager().getCurrentAudioDevice()->getDefaultBufferSizeSamples(),
            audioDeviceManager_->getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate()
        );

        initialized_ = true;
        DBG("EngineCore: Initialization complete");

        return true;
    }
    catch (const std::exception& e) {
        DBG("EngineCore: Initialization failed - " + juce::String(e.what()));
        return false;
    }
}

void EngineCore::shutdown() {
    if (!initialized_) {
        return;
    }

    DBG("EngineCore: Shutting down");

    // Shutdown audio components
    audioRenderer_->shutdown();
    audioDeviceManager_->shutdown();

    // Clear all tracks
    trackManager_->clearAllTracks();

    initialized_ = false;
    DBG("EngineCore: Shutdown complete");
}

bool EngineCore::isInitialized() const {
    return initialized_;
}

//==========================================================================
// Transport Controls
//==========================================================================

void EngineCore::play() {
    DBG("EngineCore: Play");
    transportController_->play();
    notifyTransportChanged();
}

void EngineCore::stop() {
    DBG("EngineCore: Stop");
    transportController_->stop();
    notifyTransportChanged();
}

bool EngineCore::isPlaying() const {
    return transportController_->isPlaying();
}

void EngineCore::record() {
    DBG("EngineCore: Record");
    transportController_->record();
    notifyTransportChanged();
}

void EngineCore::stopRecording() {
    DBG("EngineCore: Stop recording");
    transportController_->stopRecording();
    notifyTransportChanged();
}

bool EngineCore::isRecording() const {
    return transportController_->isRecording();
}

void EngineCore::toggleRecording() {
    DBG("EngineCore: Toggle recording");
    transportController_->toggleRecording();
    notifyTransportChanged();
}

void EngineCore::panic() {
    DBG("EngineCore: Panic");
    transportController_->panic();
    notifyTransportChanged();
}

//==========================================================================
// Position & Looping
//==========================================================================

juce::int64 EngineCore::getPlayheadSamples() const {
    return transportController_->getPlayheadSamples();
}

double EngineCore::getPlaybackPositionBeats() const {
    return transportController_->getPlaybackPositionBeats();
}

void EngineCore::setPlayheadSamples(juce::int64 position) {
    transportController_->setPlayheadSamples(position);
}

void EngineCore::setLooping(bool shouldLoop) {
    transportController_->setLooping(shouldLoop);
}

bool EngineCore::isLooping() const {
    return transportController_->isLooping();
}

void EngineCore::setLoopRegion(juce::int64 start, juce::int64 end) {
    transportController_->setLoopRegion(start, end);
}

juce::int64 EngineCore::getLoopStart() const {
    return transportController_->getLoopStart();
}

juce::int64 EngineCore::getLoopEnd() const {
    return transportController_->getLoopEnd();
}

//==========================================================================
// Audio Processing
//==========================================================================

void EngineCore::suspendProcessing(bool shouldSuspend) {
    suspended_.store(shouldSuspend);
    audioDeviceManager_->setSuspended(shouldSuspend);
}

bool EngineCore::isSuspended() const {
    return suspended_.load();
}

double EngineCore::getCpuUsage() const {
    return audioDeviceManager_->getCpuUsage();
}

double EngineCore::getSampleRate() const {
    return sampleRate_;
}

int EngineCore::getBufferSize() const {
    return bufferSize_;
}

//==========================================================================
// Device Management
//==========================================================================

juce::AudioDeviceManager& EngineCore::getDeviceManager() {
    return audioDeviceManager_->getDeviceManager();
}

juce::String EngineCore::getAudioDeviceInfo() const {
    return audioDeviceManager_->getDeviceInfo();
}

//==========================================================================
// Event System
//==========================================================================

bool EngineCore::queueEvent(const EngineEvent& e) {
    if (eventFifo_.getFreeSpace() == 0) {
        DBG("EngineCore: Event queue full - dropping event");
        return false;
    }

    auto start1 = eventFifo_.getFifoIndex();
    auto start2 = eventFifo_.getFifoIndex();
    auto numAdded = eventFifo_.write(e.buffer, e.size);
    return numAdded > 0;
}

void EngineCore::addChangeListener(juce::ChangeListener* listener) {
    changeListeners_.addListener(listener);
}

void EngineCore::removeChangeListener(juce::ChangeListener* listener) {
    changeListeners_.removeListener(listener);
}

//==========================================================================
// Configuration
//==========================================================================

void EngineCore::setEnableTestTone(bool enabled) {
    audioRenderer_->enableTestTone(enabled);
}

bool EngineCore::isTestToneEnabled() const {
    return audioRenderer_->isTestToneEnabled();
}

//==========================================================================
// Project State Management
//==========================================================================

void EngineCore::setProjectState(ProjectState* state) {
    projectState_ = state;
    syncWithProjectState();
    DBG("EngineCore: Project state set");
}

void EngineCore::syncWithProjectState() {
    if (!projectState_) {
        DBG("EngineCore: No project state to sync");
        return;
    }

    DBG("EngineCore: Syncing with project state");

    // TODO: Implement actual project state synchronization
    // This would involve:
    // - Loading tracks from project state
    // - Setting up routing
    // - Loading plugin settings
    // - Loading automation data

    notifyTrackStateChanged();
}

void EngineCore::syncTempoMap() {
    // TODO: Implement tempo map synchronization
    DBG("EngineCore: Syncing tempo map");
}

//==========================================================================
// Audio Processing Callback
//==========================================================================

void EngineCore::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context) noexcept {

    processAudioBlock(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples, context);
}

void EngineCore::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
    DBG("EngineCore: MIDI message received");
    // TODO: Route MIDI to appropriate tracks
}

//==========================================================================
// Processing Helpers
//==========================================================================

void EngineCore::processAudioBlock(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context) {

    if (suspended_.load()) {
        // Clear output if suspended
        for (int channel = 0; channel < numOutputChannels; ++channel) {
            if (outputChannelData[channel] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
            }
        }
        return;
    }

    // Update transport position
    transportController_->processAudioBlock(numSamples);

    // Get current track snapshots
    auto tracks = trackManager_->getTrackPointersSnapshot();
    auto auxBuses = std::vector<AuxBus*>(); // TODO: Get aux bus snapshots

    // Process audio
    audioRenderer_->processAudioBlock(
        inputChannelData,
        numInputChannels,
        outputChannelData,
        numOutputChannels,
        numSamples,
        transportController_->getPlayheadSamples(),
        tracks,
        auxBuses.get()
    );

    // Process events
    processEvents();
}

void EngineCore::processEvents() {
    // Process queued engine events
    int numToRead = eventFifo_.getNumReady();
    for (int i = 0; i < numToRead; ++i) {
        EngineEvent event;
        auto start1 = eventFifo_.getFifoIndex();
        auto start2 = eventFifo_.getFifoIndex();
        auto numRead = eventFifo_.read(event.buffer, event.size);

        if (numRead > 0) {
            // TODO: Process the event
            DBG("EngineCore: Processing event type " + juce::String(event.type));
        }
    }
}

//==========================================================================
// Notification Helpers
//==========================================================================

void EngineCore::notifyTransportChanged() {
    juce::ChangeBroadcaster::sendChangeMessage();
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

void EngineCore::notifyPositionChanged() {
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

void EngineCore::notifyTrackStateChanged() {
    juce::ChangeBroadcaster::sendChangeMessage();
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

//==========================================================================
// Component Initialization
//==========================================================================

void EngineCore::initializeCoreComponents() {
    DBG("EngineCore: Initializing core components");

    // Create core interface implementations
    audioDeviceManager_ = std::make_unique<AudioDeviceManager>();
    trackManager_ = std::make_unique<TrackManager>();
    transportController_ = std::make_unique<TransportController>();
    audioRenderer_ = std::make_unique<AudioRenderer>();

    DBG("EngineCore: Core components initialized");
}

void EngineCore::initializeAudioProcessing() {
    DBG("EngineCore: Initializing audio processing components");

    // TODO: Initialize AudioFilePool
    // audioFilePool_ = std::make_unique<AudioFilePool>();

    // TODO: Initialize RecordingManager
    // recordingManager_ = std::make_unique<RecordingManager>();

    // TODO: Initialize Metronome
    // metronome_ = std::make_unique<Metronome>();

    // TODO: Initialize MIDI 2.0 Discovery Service
    // midi2DiscoveryService_ = std::make_unique<Midi2DiscoveryService>(*this);

    DBG("EngineCore: Audio processing components initialized");
}

void EngineCore::initializeInstrumentsAndPlugins() {
    DBG("EngineCore: Initializing instruments and plugins");

    // Initialize Instrument Registry
    instrumentRegistry_ = std::make_unique<InstrumentRegistry>();
    zenith::registerBuiltInInstruments(*instrumentRegistry_);
    DBG("EngineCore: Instrument registry initialized with built-in instruments");

    // Initialize Plugin Host
    pluginHost_ = std::make_unique<PluginHost>();
    pluginHost_->scanDefaultLocations(true);
    DBG("EngineCore: Plugin host initialized");

    DBG("EngineCore: Instruments and plugins initialized");
}

void EngineCore::initializeAIAgents() {
    DBG("EngineCore: Initializing AI agents");

    // Initialize Session Debugger Agent
    sessionDebugger_ = std::make_unique<SessionDebuggerAgent>(*this);
    DBG("EngineCore: Session debugger agent initialized");

    // Initialize AI Mastering Agent
    masteringAgent_ = std::make_unique<AIMasteringAgent>(*this);
    DBG("EngineCore: AI mastering agent initialized");

    DBG("EngineCore: AI agents initialized");
}

} // namespace zenith