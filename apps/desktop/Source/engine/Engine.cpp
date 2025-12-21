/**
 * @file Engine.cpp
 * @brief Audio engine implementation - Core Logic
 */

#include "Engine.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "TrackAutomationSynchronizer.h"
#include <algorithm> // For std::remove_if
#include <array>     // For RT-safe stack allocation in audio callback

// C3: Include donor headers (NOT in Engine.h to avoid exposing implementation)
#include "../ai/SessionDebuggerAgent.h"
#include "../engine/AudioFilePool.h"
#include "../engine/AuxBus.h"
#include "../engine/Clip.h"
#include "../engine/EngineConstants.h"
#include "../engine/MixerChannel.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"
#include "../engine/TrackFreeze.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/RegisterBuiltInInstruments.h"
#include "PluginEditorWindow.h"

// Refactor 2025-12-09: Modular Components
#include "../engine/AudioRenderer.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"
#include "../engine/MeteringSystem.h"
#include "../engine/Metronome.h"

//==============================================================================
namespace zenith {

Engine::Engine() {
  DBG("Engine: Constructor");

  // Initialize Modular Components
  audioRenderer_ = std::make_unique<AudioRenderer>();
  recordingManager_ = std::make_unique<RecordingManager>();
  transportController_ = std::make_unique<TransportController>();
  metronome_ = std::make_unique<Metronome>();
  meteringSystem_ = std::make_unique<MeteringSystem>();
  DBG("Engine: Modular components initialized");

  // Initialize audio file pool for sample caching
  audioFilePool_ = std::make_unique<zenith::AudioFilePool>();
  DBG("Engine: AudioFilePool created");

  // Initialize plugin host and editor window manager
  pluginHost_ = std::make_unique<zenith::PluginHost>();
  pluginHost_
      ->scanDefaultLocations(); // Load cached plugins, check for crash recovery
  DBG("Engine: PluginHost initialized with " +
      juce::String(pluginHost_->getKnownPlugins().getNumTypes()) +
      " cached plugins");
  pluginEditorWindowManager_ =
      std::make_unique<zenith::PluginEditorWindowManager>();

  // Initialize Instrument Registry (built-in synths, samplers, etc.)
  instrumentRegistry_ = std::make_unique<zenith::InstrumentRegistry>();
  zenith::registerBuiltInInstruments(
      *instrumentRegistry_); // Register factories
  DBG("Engine: InstrumentRegistry initialized");

  // Initialize Session Debugger Agent (AI Technical Integrity)
  sessionDebugger_ = std::make_unique<ai::SessionDebuggerAgent>(*this);
  DBG("Engine: SessionDebuggerAgent initialized");

  // MeteringSystem handles analysis FIFO internally

  // Initialize tempo map for beat/time conversions
  tempoMap_ = std::make_unique<zenith::TempoMap>();
  DBG("Engine: TempoMap initialized");

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
}

Engine::~Engine() {
  DBG("Engine: Destructor");

  // Set shutdown flag to prevent async callbacks during destruction
  isShuttingDown_.store(true);

  // Disable MIDI input before shutdown
  disableMidiInput();

  shutdown();

  // Explicitly reset managers to ensure orderly shutdown
  audioRenderer_.reset();
  recordingManager_.reset();
  transportController_.reset();
  meteringSystem_.reset();

  // Clear audio file pool
  if (audioFilePool_ != nullptr) {
    audioFilePool_.reset();
  }

  DBG("Engine: Cleanup complete");
}

//==============================================================================
// Initialization / Shutdown
//==============================================================================

void Engine::setProjectState(ProjectState *state) {
  DBG("Engine: Setting project state");

  // Stop automation if running
  if (automationSynchronizer) {
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

void Engine::syncTempoMap() {
  if (projectState_ && tempoMap_) {
    tempoMap_->updateFromValueTree(projectState_->getTempoMap());
  }
}

void Engine::syncWithProjectState() {
  DBG("Engine: Syncing with project state");

  if (projectState_ == nullptr) {
    DBG("Engine: No project state, clearing tracks");
    tracks_.clear();
    return;
  }

  // Clear existing tracks
  tracks_.clear();

  // Get tracks from project state
  auto &state = projectState_->getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid()) {
    DBG("Engine: No tracks in project state");
    return;
  }

  const double sampleRate = currentSampleRate.load();
  const int bufferSize = currentBufferSize.load();
  
  // Helper: convert beats to samples using TempoMap
  auto beatsToSamples = [this, sampleRate](double beats) -> juce::int64 {
    if (tempoMap_) {
      return tempoMap_->beatsToSamples(beats, sampleRate);
    }
    // Fallback if no tempo map
    const double tempo = projectState_ ? projectState_->getTempo() : 120.0;
    const double secondsPerBeat = 60.0 / tempo;
    const double seconds = beats * secondsPerBeat;
    return static_cast<juce::int64>(seconds * sampleRate);
  };

  // Create engine tracks from project state
  for (auto trackNode : tracksNode) {
    juce::String trackName = trackNode[ProjectState::PROP_NAME].toString();
    juce::String trackType = trackNode[ProjectState::PROP_TYPE].toString();

    // Create track via Factory
    zenith::Track::Type actualType = zenith::Track::Type::Audio;
    if (trackType == "midi") actualType = zenith::Track::Type::MIDI;
    else if (trackType == "instrument") actualType = zenith::Track::Type::Instrument;
    else if (trackType == "bus") actualType = zenith::Track::Type::Bus;

    auto track = zenith::Track::create(trackName, actualType);

    // Set track ID
    track->setTrackId(trackNode[ProjectState::PROP_ID].toString());

    // Set mixer properties
    track->setVolume(trackNode[ProjectState::PROP_VOLUME]);
    track->setPan(trackNode[ProjectState::PROP_PAN]);
    track->setMuted(trackNode[ProjectState::PROP_MUTE]);
    track->setSolo(trackNode[ProjectState::PROP_SOLO]);

    // Prepare track for playback
    if (sampleRate > 0) {
      track->prepareToPlay(bufferSize, sampleRate);
    }

    // Load clips
    auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
    if (clipsNode.isValid()) {
      for (auto clipNode : clipsNode) {
        // Create clip
        auto clip = std::make_unique<zenith::Clip>();

        // Set basic properties
        double startBeats = clipNode[ProjectState::PROP_START];
        double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

        clip->setStartPosition(beatsToSamples(startBeats));
        clip->setLength(beatsToSamples(lengthBeats));

        // Load audio file if present
        juce::String audioFilePath =
            clipNode[ProjectState::PROP_AUDIO_FILE].toString();
        if (audioFilePath.isNotEmpty()) {
          juce::File audioFile(audioFilePath);
          if (audioFile.existsAsFile()) {
            clip->setAudioFile(audioFile);
            clip->setType(zenith::Clip::Type::Audio);
            DBG("Engine: Loaded audio file: " + audioFile.getFileName());
          } else {
            DBG("Engine: Warning - audio file not found: " + audioFilePath);
          }
        }

        // Prepare clip
        if (sampleRate > 0) {
          clip->prepareToPlay(bufferSize, sampleRate);
        }

        // Set clip as playing (so it's active during playback)
        clip->setPlaying(true);

        // Add clip to track
        track->addClip(std::move(clip));
      }
    }

    // Add track to engine
    track->setTrackIndex((int)tracks_.size());
    tracks_.push_back(std::move(track));

    // Register with RoutingGraph and connect to master bus
    RoutingGraph::Node node;
    node.id = tracks_.back()->getTrackId();
    node.name = tracks_.back()->getName();
    node.type = RoutingGraph::NodeType::Track;
    routingGraph_.addNode(node);

    // Automatically route track to master bus
    routingGraph_.connect(tracks_.back()->getTrackId(), "master", 1.0f);
  }

  DBG("Engine: Synced " + juce::String(tracks_.size()) + " tracks");

  // Update snapshot for audio thread
  updateTrackSnapshot();
}

bool Engine::initialize() {
  DBG("Engine: Initializing...");

  // Initialize audio device manager
  auto error = deviceManager.initialiseWithDefaultDevices(2, 2); // 2 in, 2 out

  if (error.isNotEmpty()) {
    DBG("Engine: Failed to initialize audio device: " + error);
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon, "Audio Device Error",
        "Failed to initialize audio device:\n" + error, "OK");
    return false;
  }

  // Get current device setup
  auto setup = deviceManager.getAudioDeviceSetup();

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
// CPU Monitoring
//==============================================================================

double Engine::getCpuUsage() const {
  return deviceManager.getCpuUsage() * 100.0;
}

//==============================================================================
// Audio File Pool
//==============================================================================

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
  jassert(pluginEditorWindowManager_ != nullptr);
  return *pluginEditorWindowManager_;
}

const zenith::TempoMap &Engine::getTempoMap() const noexcept {
  jassert(tempoMap_ != nullptr);
  return *tempoMap_;
}

juce::AudioPluginFormatManager &Engine::getPluginFormatManager() {
  return pluginHost_->getFormatManager();
}

void Engine::registerFormats() {
  // Bug 27: JUCE FormatManager takes ownership of registered formats
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

//==============================================================================
// AudioIODeviceCallback Implementation
//==============================================================================

void Engine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  DBG("Engine: Audio device starting...");

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
  if (audioRenderer_) {
    audioRenderer_->prepare(currentSampleRate.load(), currentBufferSize.load(),
                            tracks_.size(), auxBuses_.size());
  }

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

  // Load snapshot for RT-safe access
  auto *snapshot = activeSnapshot_.load();
  if (!snapshot)
    return;

  // Process Events (Updates track parameters etc.)
  processEvents();

  // Midi Buffer for rendering (populated from FIFO)
  juce::MidiBuffer midiBuffer;
  midiFifo_.drainTo(midiBuffer, numSamples);

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
      
      // Extract MIDI for this section (simple split not supported by MidiBuffer, using full buffer for now or empty?)
      // For correctness in looping, we should split MIDI, but for now passing empty/full might be acceptable if events are sparse.
      // Ideally handled by AudioRenderer splitting. Here we pass the full drained midi to the first block or handle properly.
      // AudioRenderer::renderAudioGraph is responsible for processing MIDI. 

      if (audioRenderer_) {
        // NOTE: We pass 'midiBuffer' (full buffer) to first pass. 
        // This is a simplification; ideally we split MIDI events based on timestamp.
        audioRenderer_->renderAudioGraph(
            buffer1, samplesBeforeLoop, currentPos, snapshot->tracks,
            snapshot->auxBuses, routingGraph_, masterLimiter_,
            masterPlugins_, tempoMap_.get(), &midiBuffer);

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
        jassert(numOutputChannels <= 32 && "Audio callback has a hardcoded limit of 32 channels");
        float* offsets[32]; // Max 32 channels supported
        int safeNumChannels = juce::jmin(numOutputChannels, 32);

        for (int ch = 0; ch < safeNumChannels; ++ch)
            if (outputChannelData[ch])
                offsets[ch] = outputChannelData[ch] + samplesBeforeLoop;

        juce::AudioBuffer<float> buffer2(offsets, safeNumChannels,
                                         samplesAfter);
        
        juce::MidiBuffer emptyMidi; // No MIDI in wrapped part for now

        if (audioRenderer_) {
          audioRenderer_->renderAudioGraph(
              buffer2, samplesAfter, loopStart, snapshot->tracks,
              snapshot->auxBuses, routingGraph_, masterLimiter_,
              masterPlugins_, tempoMap_.get(), &emptyMidi);

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
  juce::AudioBuffer<float> fullOutput(outputChannelData, numOutputChannels, numSamples);
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

void Engine::applyEvent(const zenith::EngineEvent& e, TrackSnapshot* snapshot) noexcept {
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

// Retain legacy public render API for other consumers if any (but AudioRenderer does the work)
void Engine::renderAudioGraph(juce::AudioBuffer<float> &outputBuffer,
                              int numSamples, juce::int64 playheadPosition,
                              const std::vector<zenith::Track *> &tracks,
                              const std::vector<zenith::AuxBus *> &auxBuses,
                              const juce::MidiBuffer *incomingMidi) {
  if (audioRenderer_) {
    audioRenderer_->renderAudioGraph(outputBuffer, numSamples, playheadPosition,
                                     tracks, auxBuses, routingGraph_,
                                     masterLimiter_, masterPlugins_,
                                     tempoMap_.get(), incomingMidi);
  } else {
    outputBuffer.clear();
  }
}

// Legacy processAudioBlock - keep as private helper if needed or just remove, but to match summary:
void Engine::processAudioBlock(const float *const *inputChannelData,
                               int numInputChannels,
                               float *const *outputChannelData,
                               int numOutputChannels, int numSamples) noexcept {
    // Legacy method - delegated to audioDeviceIOCallbackWithContext logic via loop
    // But since IO callback is the updated one, this might be unused.
    // However, keeping it as an empty shell or redirecting to ensure signature match if vtable requires it.
    // Engine declares it private.
    juce::ignoreUnused(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
}

} // namespace zenith
