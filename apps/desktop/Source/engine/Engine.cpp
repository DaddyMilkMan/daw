/**
 * @file Engine.cpp
 * @brief Audio engine implementation - Core Logic
 */

#include "Engine.h"
#include "MixerController.h"
#include "PlatformAudioUtils.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include "ZenithLogger.h"
#include "../Settings.h"
#include <algorithm> // For std::remove_if
#include <array>     // For RT-safe stack allocation in audio callback
#include <memory>    // For std::make_unique

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../ai/AIMasteringAgent.h"
#include "../ai/SessionDebuggerAgent.h"
#include "../engine/AudioFilePool.h"
#include "../engine/AuxBus.h"
#include "../engine/Clip.h"
#include "../engine/EngineConstants.h"
#include "../engine/MixerChannel.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../engine/TrackFreeze.h"
#include "../engine/Midi2DiscoveryService.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include "PluginEditorWindow.h"

// Refactor 2025-12-09: Modular Components
#include "../engine/AudioRenderer.h"
#include "../engine/AudioExporter.h"
#include "../engine/MeteringSystem.h"
#include "../engine/Metronome.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"
#include "RTSafetyChecks.h"  // Zero-Latency Agent: RT-safety debug infrastructure

//==============================================================================
namespace zenith {

Engine::Engine() {
  DBG("Engine: Constructor");

  // Initialize Modular Components
  audioRenderer_ = std::make_unique<AudioRenderer>();
  audioExporter_ = std::make_unique<AudioExporter>(*this);
  recordingManager_ = std::make_unique<RecordingManager>();
  transportController_ = std::make_unique<TransportController>();
  metronome_ = std::make_unique<Metronome>();
  meteringSystem_ = std::make_unique<MeteringSystem>();
  mixerController_ = std::make_unique<MixerController>(*this);
  DBG("Engine: Modular components initialized");

  // Initialize audio file pool for sample caching
  audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
  DBG("Engine: AudioFilePool created");

  // Initialize plugin host and editor window manager
  pluginHost_ = std::make_unique<zenith::PluginHost>();
  pluginHost_->scanDefaultLocations(true); // Load cached plugins, check for crash asynchronously
                                       // recovery
  DBG("Engine: PluginHost initialized with " +
      juce::String(pluginHost_->getKnownPlugins().getNumTypes()) +
      " cached plugins");
  // NOTE: PluginEditorWindowManager is lazy-loaded on first use via 
  // getPluginEditorWindowManager() to avoid GUI dependencies in headless tests.

  // Initialize Instrument Registry (built-in synths, samplers, etc.)
  instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
  zenith::registerBuiltInInstruments(
      *instrumentRegistry_); // Register factories
  DBG("Engine: InstrumentRegistry initialized");

  // Initialize Session Debugger Agent (AI Technical Integrity)
  sessionDebugger_ = std::make_unique<ai::SessionDebuggerAgent>(*this);
  DBG("Engine: SessionDebuggerAgent initialized");

  // Initialize AI Mastering Agent
  masteringAgent_ = std::make_unique<ai::AIMasteringAgent>(*this);
  DBG("Engine: AIMasteringAgent initialized");

  // MeteringSystem handles analysis FIFO internally

  // Initialize tempo map for beat/time conversions
  tempoMap_ = std::make_unique<zenith::TempoMap>();
  DBG("Engine: TempoMap initialized");

  // Initialize MIDI 2.0 Discovery Service
  midi2DiscoveryService_ = std::make_unique<Midi2DiscoveryService>(*this);
  DBG("Engine: Midi2DiscoveryService initialized");

  // Wire up transport controller to tempo map
  transportController_->setTempoMap(tempoMap_.get());

  // Initialize TrackFreezeManager for CPU optimization
  freezeManager_ = std::make_unique<TrackFreezeManager>();
  DBG("Engine: TrackFreezeManager initialized");

  // Register Master Bus node in RoutingGraph
  // This is the final destination for all audio before output
  RoutingGraph::Node masterNode;
  masterNode.id = "master";
  masterNode.name = "Master";
  masterNode.type = RoutingGraph::NodeType::Master;
  masterNode.channelCount = 2;
  routingGraph_.addNode(masterNode);
  DBG("Engine: Master bus registered in RoutingGraph");

    // Initialize track snapshot
    updateTrackSnapshot();

    DBG("Engine: Constructor complete");
}

Engine::~Engine() {
  ZENITH_LOG_INFO("Engine: Destructor STARTED");
  DBG("Engine: Destructor");

  // Set shutdown flag to prevent async callbacks during destruction
  isShuttingDown_.store(true);

  // Stop automation if running
  if (automationSynchronizer) {
    ZENITH_LOG_INFO("Engine: Stopping automationSynchronizer...");
    automationSynchronizer->stop();
    automationSynchronizer.reset();
  }

  // Disable MIDI input before shutdown
  ZENITH_LOG_INFO("Engine: Disabling MIDI input...");
  disableMidiInput();

  shutdown();

  // Explicitly reset managers to ensure orderly shutdown
  ZENITH_LOG_INFO("Engine: Resetting modular components...");
  audioRenderer_.reset();
  recordingManager_.reset();
  transportController_.reset();
  meteringSystem_.reset();
  masteringAgent_.reset();

  // Clear audio file pool
  if (audioFilePool_ != nullptr) {
    ZENITH_LOG_INFO("Engine: Resetting audioFilePool...");
    audioFilePool_.reset();
  }

  ZENITH_LOG_INFO("Engine: Destructor COMPLETE");
  DBG("Engine: Cleanup complete");
}

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState *state) {
  DBG("Engine: Setting project state");

  // Stop automation if running
  if (automationSynchronizer) {
    ZENITH_LOG_INFO("Engine: Stopping old automationSynchronizer...");
    automationSynchronizer->stop();
    automationSynchronizer.reset();
  }

  projectState_ = state;

  // Wire up recording manager with project state
  if (recordingManager_) {
    recordingManager_->setProjectState(projectState_);
    DBG("Engine: RecordingManager wired to ProjectState");
  }

  // Create new automation synchronizer if we have a project state
  if (projectState_ != nullptr) {
    automationSynchronizer =
        std::make_unique<TrackAutomationSynchronizer>(*projectState_, *this);
    DBG("Engine: Created automation synchronizer");

    // Sync tracks with project state
    syncWithProjectState();

    // Sync tempo map
    syncTempoMap();
  }
}

bool Engine::initialize() {
  DBG("Engine: Initializing...");

  // 1. Initialize Audio Device Manager with default devices
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2); // 2 in, 2 out

  if (error.isNotEmpty()) {
    DBG("Engine: Failed to initialize audio device: " + error);
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Audio Device Error",
        "Failed to initialize audio device:\n" + error, "OK");
    // Don't return false yet, try platform-specific fallback
  }

  // 2. Initialize Audio Devices with platform-specific fallbacks
  PlatformAudioUtils::initializeAudioDeviceSetup(deviceManager);

  // Get current device setup (might have changed due to fallback)
  auto setup = deviceManager.getAudioDeviceSetup();

  DBG("========================================================================"
      "=="
      "=====================");

  // Refresh setup if it changed during fallback
  setup = deviceManager.getAudioDeviceSetup();

  DBG("Engine: Audio device initialized");
  DBG("  Device: " + setup.outputDeviceName);
  DBG("  Sample Rate: " + juce::String(setup.sampleRate) + " Hz");
  DBG("  Buffer Size: " + juce::String(setup.bufferSize) + " samples");

  // Store settings
  currentSampleRate.store(setup.sampleRate);
  currentBufferSize.store(setup.bufferSize);

  // Add this engine as the audio callback
  deviceManager.addAudioCallback(this);

  // Wire up recording manager with device manager for input channel info
  if (recordingManager_) {
    recordingManager_->setDeviceManager(&deviceManager);
    recordingManager_->prepare(setup.sampleRate);
    DBG("Engine: RecordingManager wired to DeviceManager");
  }

  // Prepare Metronome
  if (metronome_) {
    metronome_->prepareToPlay(setup.sampleRate, setup.bufferSize);
    // Sync metronome state with TransportController
    metronome_->setEnabled(transportController_->isMetronomeEnabled());
    metronome_->setLevel(transportController_->getMetronomeLevel());
  }

  // Enable MIDI input devices
  enableMidiInput();

// Optional debug seed (disabled by default; enable with
// -DZENITH_ENGINE_SEED_DEBUG_TRACKS=ON)
#if defined(JUCE_DEBUG) && defined(ZENITH_ENGINE_SEED_DEBUG_TRACKS)
  DBG("Engine: Seeding debug tracks (ZENITH_ENGINE_SEED_DEBUG_TRACKS enabled)");
  addTestTracks(8);
#endif

  // Start Session Debugger monitoring (AI Technical Integrity Agent)
  if (sessionDebugger_) {
    sessionDebugger_->startMonitoring(500); // Analyze every 500ms
    DBG("Engine: SessionDebugger monitoring started");
  }

  DBG("Engine: Initialization complete!");
  return true;
}

void Engine::shutdown() {
  ZENITH_LOG_INFO("Engine::shutdown() STARTED");
  DBG("Engine: Shutting down...");

  // Stop Session Debugger monitoring first
  if (sessionDebugger_) {
    sessionDebugger_->stopMonitoring();
    DBG("Engine: SessionDebugger monitoring stopped");
  }

  // Stop playback
  stop();

  // Remove audio callback
  deviceManager.removeAudioCallback(this);

  // Close audio device
  deviceManager.closeAudioDevice();

  // Clear audio file pool
  if (audioFilePool_) {
    audioFilePool_->clear();
  }

  ZENITH_LOG_INFO("Engine: shutdown() COMPLETE");
  DBG("Engine: Shutdown complete");
}

//==============================================================================
// Audio Device Management
//==============================================================================

juce::String Engine::getAudioDeviceInfo() const {
  auto *device = deviceManager.getCurrentAudioDevice();

  if (device == nullptr)
    return "No device";

  auto name = device->getName();
  auto sampleRate = device->getCurrentSampleRate();
  auto bufferSize = device->getCurrentBufferSizeSamples();

  return name + " @ " + juce::String(sampleRate, 0) + " Hz, " +
         juce::String(bufferSize) + " samples";
}

//==============================================================================
// Change Listener Callback (PDC Updates)
//==============================================================================

void Engine::changeListenerCallback(juce::ChangeBroadcaster* source) {
    // Check if the change comes from a track (plugin added/removed/latency changed)
    if (auto* track = dynamic_cast<zenith::Track*>(source)) {
        // Trigger PDC recalculation on the message thread
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            recalculatePDC();
        } else {
            // If this happens on another thread (unlikely for plugins but possible), 
            // dispatch to message thread
            juce::MessageManager::callAsync([this]() {
                recalculatePDC();
            });
        }
    }
}

zenith::AudioFilePool &Engine::getAudioFilePool() {
  jassert(audioFilePool_ != nullptr);
  return *audioFilePool_;
}

//==============================================================================
// Plugin Hosting
//==============================================================================

zenith::PluginHost &Engine::getPluginHost() noexcept {
  jassert(pluginHost_ != nullptr);
  return *pluginHost_;
}

int Engine::scanForPlugins() {
  if (pluginHost_ == nullptr) {
    DBG("Engine: PluginHost not initialized");
    return 0;
  }

  DBG("Engine: Scanning for plugins...");
  int count = pluginHost_->scanDefaultLocations();
  DBG("Engine: Plugin scan complete - found " + juce::String(count) +
      " plugins");

  return count;
}

zenith::PluginEditorWindowManager &
Engine::getPluginEditorWindowManager() noexcept {
  // Lazy initialization to avoid GUI dependencies in headless tests
  if (!pluginEditorWindowManager_) {
    pluginEditorWindowManager_ = std::make_unique<zenith::PluginEditorWindowManager>();
    DBG("Engine: PluginEditorWindowManager created (lazy)");
  }
  return *pluginEditorWindowManager_;
}

zenith::MixerController& Engine::getMixerController() {
    jassert(mixerController_ != nullptr);
    return *mixerController_;
}

// getTempoMap defined in EngineSync.cpp

juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}

void Engine::registerFormats() {
  // Bug 27: JUCE FormatManager takes ownership of registered formats
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

// setTrackPan, setTrackInputChannel, setTrackMute defined in EngineMixing.cpp

// setTrackSolo, setTrackArmed defined in EngineMixing.cpp

//==============================================================================
// Metering functions defined in EngineMixing.cpp:
//   getTrackLevel, getTrackPeakLevel, getMasterLevel, getMasterPeakLevel,
//   resetPeakMeters
//==============================================================================

// createTrack defined in EngineTrackManagement.cpp

// addTrack defined in EngineTrackManagement.cpp

// removeTrack defined in EngineTrackManagement.cpp

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  DBG("Engine: Audio device starting...");

  // Zero-Latency Agent: Mark this thread for RT-safety assertions
  rt::markAsAudioThread();

  // Update settings
  currentSampleRate.store(device->getCurrentSampleRate());
  currentBufferSize.store(device->getCurrentBufferSizeSamples());

  // Reset state
  phase = 0.0;
  if (transportController_) {
    transportController_->setPlayheadSamples(0);
  }

  // Phase 1: Prepare tracks for audio processing
  prepareTracks(device->getCurrentBufferSizeSamples(),
                device->getCurrentSampleRate());

  // Prepare Aux Buses
  for (auto &bus : auxBuses_) {
    if (bus) {
      bus->prepareToPlay(device->getCurrentBufferSizeSamples(),
                         currentSampleRate.load());
    }
  }

  // Prepare AudioRenderer (Handles buffers, PDC, metering, limiter)
  renderContext_.prepare(device->getCurrentSampleRate(),
                         device->getCurrentBufferSizeSamples(),
                         tracks_.size(), auxBuses_.size());

  // Prepare MasterLimiter (owned by Engine, used by AudioRenderer via ref)
  masterLimiter_.prepare(currentSampleRate.load(), currentBufferSize.load());
  masterLimiter_.setCeiling(constants::kDefaultLimiterCeilingDb);
  masterLimiter_.setEnabled(true);

  // Prepare RecordingManager
  if (recordingManager_) {
    recordingManager_->prepare(currentSampleRate.load());
  }

  // Note: MasterLimiter is now managed by AudioRenderer

  DBG("Engine: Audio device started");
  DBG("  Sample Rate: " + juce::String(currentSampleRate.load()) + " Hz");
  DBG("  Buffer Size: " + juce::String(currentBufferSize.load()) + " samples");
}

void Engine::audioDeviceStopped() {
  DBG("Engine: Audio device stopped");

  // Phase 11: Release resources from all tracks
  for (auto &track : tracks_) {
    if (track != nullptr) {
      track->releaseResources();
    }
  }
}

void Engine::audioDeviceIOCallbackWithContext(
    const float *const *inputChannelData, int numInputChannels,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext &context) noexcept {

  juce::ignoreUnused(inputChannelData, numInputChannels, context);

  // Clean output buffers
  for (int i = 0; i < numOutputChannels; ++i) {
    if (outputChannelData[i]) {
      juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
  }

  // Load snapshots for RT-safe access
  auto *snapshot = activeSnapshot_.load();
  auto *masterSnapshot = activeMasterPluginsSnapshot_.load();
  if (!snapshot)
    return;

  // Process Events (Updates track parameters etc.)
  processEvents();

  // Midi Buffer for rendering (populated from FIFO)
  juce::MidiBuffer midiBuffer;
  midiFifo_.drainTo(midiBuffer, numSamples);

  // Master plugins for pass 1/2
  std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins;
  if (masterSnapshot) masterPlugins = masterSnapshot->plugins;

  if (transportController_ && transportController_->isPlaying()) {
    juce::int64 currentPos = transportController_->getPlayheadSamples();
    juce::int64 loopEnd = transportController_->getLoopEndSamples();
    juce::int64 loopStart = transportController_->getLoopStartSamples();
    bool looping = transportController_->isLooping();

    // Check for loop wrap
    bool wrapped = false;
    int samplesBeforeLoop = numSamples;

    if (looping && loopEnd > 0 && loopEnd > loopStart && currentPos < loopEnd &&
        (currentPos + numSamples) > loopEnd) {
      wrapped = true;
      samplesBeforeLoop = static_cast<int>(loopEnd - currentPos);
    }

    // Pass 1
    if (samplesBeforeLoop > 0) {
      // Use proxy buffer to avoid allocation
      juce::AudioBuffer<float> buffer1(outputChannelData, numOutputChannels,
                                       samplesBeforeLoop);
      juce::MidiBuffer midi1;
      midiFifo_.drainTo(midi1, samplesBeforeLoop);

      if (audioRenderer_) {
        audioRenderer_->renderAudioGraph(
            renderContext_,
            buffer1, samplesBeforeLoop, currentPos, snapshot->tracks,
            snapshot->auxBuses, routingGraph_, masterLimiter_, masterPlugins,
            tempoMap_.get(), &midi1, inputChannelData, numInputChannels);

        // Mix Metronome (Pass 1)
        if (metronome_) {
          metronome_->getNextAudioBlock(buffer1, currentPos, true, *tempoMap_);
        }
      }
    }

    // Pass 2 (Wrapped)
    if (wrapped) {
      int samplesAfter = numSamples - samplesBeforeLoop;
      if (samplesAfter > 0) {
        // Use stack array for channel pointers to avoid heap allocation
        jassert(numOutputChannels <= 32 &&
                "Audio callback has a hardcoded limit of 32 channels");
        float *offsets[32]; // Max 32 channels supported
        int safeNumChannels = juce::jmin(numOutputChannels, 32);

        for (int ch = 0; ch < safeNumChannels; ++ch)
          if (outputChannelData[ch])
            offsets[ch] = outputChannelData[ch] + samplesBeforeLoop;

        juce::AudioBuffer<float> buffer2(offsets, safeNumChannels,
                                         samplesAfter);

        juce::MidiBuffer emptyMidi; // No MIDI in wrapped part for now

        if (audioRenderer_) {
          audioRenderer_->renderAudioGraph(
              renderContext_,
              buffer2, samplesAfter, loopStart, snapshot->tracks,
              snapshot->auxBuses, routingGraph_, masterLimiter_, masterPlugins,
              tempoMap_.get(), &emptyMidi, offsets,
              safeNumChannels); // Using offset inputs

          // Mix Metronome (Pass 2)
          if (metronome_) {
            metronome_->getNextAudioBlock(buffer2, loopStart, true, *tempoMap_);
          }
        }

        transportController_->setPlayheadSamples(loopStart + samplesAfter);
      }
    } else {
      transportController_->advancePlayhead(numSamples);
    }
  } else {
    // Not playing?
    // We could process reverb tails here if we wanted.
  }

  // Analysis / Visualizers
  // We need to push the final output to the analysis FIFO.
  // Reconstruct full buffer wrapper
  juce::AudioBuffer<float> fullOutput(outputChannelData, numOutputChannels,
                                      numSamples);
  if (meteringSystem_) {
    meteringSystem_->getAnalysisFifo().push(fullOutput, numSamples);
  }

  // Audio Recording
  if (recordingManager_ && recordingManager_->isRecording()) {
    recordingManager_->captureAudio(inputChannelData, numInputChannels,
                                    numSamples, snapshot->lifecycle);
  }
}

//==============================================================================
// Real-time Event Queue
//==============================================================================

bool Engine::queueEvent(const zenith::EngineEvent &e) {
  int start1, size1, start2, size2;
  commandFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    commandBuffer_[start1] = e;
    commandFifo_.finishedWrite(1);
    return true;
  }

  return false; // Buffer full
}

void Engine::processEvents() noexcept {
  int start1, size1, start2, size2;
  commandFifo_.prepareToRead(commandFifo_.getNumReady(), start1, size1, start2,
                             size2);

  auto *snapshot = activeSnapshot_.load();

  if (size1 > 0) {
    for (int i = 0; i < size1; ++i) {
      applyEvent(commandBuffer_[start1 + i], snapshot);
    }
  }
  if (size2 > 0) {
    for (int i = 0; i < size2; ++i) {
      applyEvent(commandBuffer_[start2 + i], snapshot);
    }
  }

  commandFifo_.finishedRead(size1 + size2);
}

void Engine::applyEvent(const zenith::EngineEvent &e,
                        TrackSnapshot *snapshot) noexcept {
  if (e.type == zenith::EngineEvent::Type::SetPluginParam) {
    if (snapshot && e.trackIndex >= 0 &&
        e.trackIndex < (int)snapshot->tracks.size()) {
      auto *track = snapshot->tracks[e.trackIndex];
      if (track) {
        auto *plugin = track->getPlugin(e.pluginIndex);
        if (plugin) {
          auto params = plugin->getParameters();
          if (e.paramIndex >= 0 && e.paramIndex < (int)params.size()) {
            params[e.paramIndex]->setValueNotifyingHost(e.value);
          }
        }
      }
    }
  } else if (e.type == zenith::EngineEvent::Type::SetTrackVolume) {
    if (snapshot && e.trackIndex >= 0 &&
        e.trackIndex < (int)snapshot->tracks.size()) {
      if (auto *track = snapshot->tracks[e.trackIndex]) {
        track->setVolume(e.value);
      }
    }
  } else if (e.type == zenith::EngineEvent::Type::SetTrackPan) {
    if (snapshot && e.trackIndex >= 0 &&
        e.trackIndex < (int)snapshot->tracks.size()) {
      if (auto *track = snapshot->tracks[e.trackIndex]) {
        track->setPan(e.value);
      }
    }
  } else if (e.type == zenith::EngineEvent::Type::SetTrackMute) {
    if (snapshot && e.trackIndex >= 0 &&
        e.trackIndex < (int)snapshot->tracks.size()) {
      if (auto *track = snapshot->tracks[e.trackIndex]) {
        track->setMuted(e.boolValue);
      }
    }
  } else if (e.type == zenith::EngineEvent::Type::SetTrackSolo) {
    if (snapshot && e.trackIndex >= 0 &&
        e.trackIndex < (int)snapshot->tracks.size()) {
      if (auto *track = snapshot->tracks[e.trackIndex]) {
        track->setSolo(e.boolValue);
      }
    }
  }
}

// Retain legacy public render API for other consumers if any (but AudioRenderer
// does the work)

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Engine::processAudioBlock(const float *const *inputChannelData,
                               int numInputChannels,
                               float *const *outputChannelData,
                               int numOutputChannels, int numSamples) noexcept {
  // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
  //
  // CRITICAL WARNING: Accessing 'tracks_' (std::vector) here is NOT
  // thread-safe! If the message thread adds/removes tracks while this runs, the
  // vector may reallocate, causing a segfault.
  //
  // Phase 11: Process all tracks and mix them down to master output

  juce::ignoreUnused(inputChannelData, numInputChannels);

  // Wrap output buffer for unified render path
  juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels,
                                        numSamples);

  // Get current transport position
  juce::int64 position =
      transportController_ ? transportController_->getPlayheadSamples() : 0;

  // Update transport position for all clips in all tracks
  // Get thread-safe snapshot (Lock-free load)
  auto *snapshot = activeSnapshot_.load();

  // Process queued events (Parameter changes, etc.)
  processEvents();

  // Update transport position for all clips in all tracks (Delegated to
  // AudioRenderer)
  if (snapshot && audioRenderer_) {
    audioRenderer_->updateClipPositions(snapshot->tracks, position);
  }

  // REMOVED: Fall-through renderAudioGraph call that caused double processing
  // The logic inside transportController_->isPlaying() block now handles all
  // rendering.

  // Push to Analysis FIFO (Stereo)
  if (meteringSystem_) {
    meteringSystem_->getAnalysisFifo().push(outputBuffer, numSamples);
  }

  // Fallback: If no tracks or all tracks are silent, optionally enable test
  // tone (Only if explicitly enabled via enableTestTone_)
  // Using snapshot->tracks to check emptiness safely
  bool testToneEnabled = enableTestTone_.load();
  bool tracksEmpty = snapshot ? snapshot->tracks.empty() : true;

  if (testToneEnabled && tracksEmpty) {
    // Generate 440 Hz sine wave at -12 dB (only if no tracks exist)
    const double sampleRate = currentSampleRate.load();
    const double frequency = 440.0; // A4
    const double amplitude = 0.25;  // -12 dB
    // Safeguard against zero sample rate to avoid FPE
    const double safeSampleRate = std::max(1.0, sampleRate);
    const double phaseIncrement =
        frequency * 2.0 * juce::MathConstants<double>::pi / safeSampleRate;
    
    // Smooth the phase transition if sample rate changed?
    // For a test tone, simplistic is fine.
    
    for (int i = 0; i < numSamples; ++i) {
      float value = static_cast<float>(std::sin(phase) * amplitude);
      for (int channel = 0; channel < numOutputChannels; ++channel) {
        if (outputChannelData[channel]) {
          outputChannelData[channel][i] += value;
        }
      }
      phase += phaseIncrement;
      if (phase >= 2.0 * juce::MathConstants<double>::pi)
        phase -= 2.0 * juce::MathConstants<double>::pi;
    }
  }
}

// NOTE: renderOfflineBlock() is implemented in EngineExport.cpp

//==============================================================================
// Track Management (MESSAGE THREAD)
//==============================================================================

//==============================================================================
// Aux Bus Management
//==============================================================================

// createAuxBus, removeAuxBus, getNumAuxBuses, getAuxBus, getAuxBusLevel, getAuxBusPeakLevel
// defined in EngineMixing.cpp

//==============================================================================
// Master Limiter API
//==============================================================================

// setMasterLimiterEnabled, isMasterLimiterEnabled, setMasterLimiterCeiling,
// getMasterLimiterGainReduction, getMasterLimiterLatency defined in EngineMixing.cpp

//==============================================================================
// Track Freeze API (CPU Optimization)
//==============================================================================

// freezeTrack, unfreezeTrack, isTrackFrozen, cancelFreeze defined in EngineMixing.cpp

//==============================================================================
// Metronome
//==============================================================================

// toggleMetronome, isMetronomeEnabled, setMetronomeLevel defined in EngineTransport.cpp

//==============================================================================
// Remote Control Settings
//==============================================================================

bool Engine::isRemoteControlAllowed() const {
    return Settings::getInstance().getAllowRemoteControl();
}

//==============================================================================

//==============================================================================
juce::ThreadPool &Engine::getThreadPool() { return threadPool; }



// Moved to EngineMixing.cpp


ai::AIMasteringAgent* Engine::getMasteringAgent() const {
  return masteringAgent_.get();
}



} // namespace zenith
